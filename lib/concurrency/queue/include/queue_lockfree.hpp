#pragma once
#ifndef _QUEUE_LOCKFREE_HPP
#define _QUEUE_LOCKFREE_HPP
#include <atomic_reference_node.hpp>
#include <memory>
#include <optional>
#include <type_traits>

namespace concurrency {
template <typename _Ty> class ConcurrentQueue;
}

template <typename _Ty> class concurrency::ConcurrentQueue {
  ConcurrentQueue(const ConcurrentQueue &) = delete;
  ConcurrentQueue &operator=(const ConcurrentQueue &) = delete;

  // Permanent terminal state for Node::data. Unlike nullptr, this tag prevents
  // a stalled producer from refilling a node after a consumer has retired it.
  // Node::data stores void*, so the tag remains an untyped address and only
  // actual data pointers are converted back to _Ty*.
  inline static char consumed_tag{};

public:
  ConcurrentQueue() : m_size(0) {
    ReferenceNode<_Ty> init_node;
    init_node.node = new Node<_Ty>;
    init_node.thread_ref_counter = 1;

    m_head.store(init_node, std::memory_order_release);
    m_tail.store(init_node, std::memory_order_release);
  }
  virtual ~ConcurrentQueue() {
    // Public clear() deliberately preserves the final dummy so the queue can
    // be reused. Once no concurrent users may remain, destruction reclaims
    // that dummy exactly once.
    clear();
    delete m_tail.load().node;
  }

public:
  // Drain all values but keep the final dummy alive and referenced by both
  // m_head and m_tail. This makes explicit clear(), repeated clear(), and
  // clear-then-reuse valid. Like destruction, clear() requires exclusive
  // lifecycle access; it is not a concurrent operation on the queue.
  void clear() {
    while (pop().has_value())
      ;
  }

  const bool empty() const {
    ReferenceNode<_Ty> old_head = m_head.load(std::memory_order_relaxed);
    return ((old_head.node == m_tail.load(std::memory_order_acquire).node)
                ? true
                : false);
  }

  // Approximate while operations overlap. Because push increments after tail
  // publication, a concurrent pop can decrement first and temporarily wrap
  // this unsigned counter to SIZE_MAX.
  const std::size_t size() const { return m_size; }

  void push(_Ty &&value) { __push(std::make_unique<_Ty>(std::move(value))); }
  void push(const _Ty &value) { __push(std::make_unique<_Ty>(value)); }

  [[nodiscard]]
  std::optional<std::unique_ptr<_Ty>> pop() {
    return __pop();
  }

  [[nodiscard]]
  bool try_pop(std::unique_ptr<_Ty> &out) {
    if (auto opt = __pop(); opt.has_value()) {
      out = std::move(opt.value());
      return true;
    }
    return false;
  }

protected:
  void __push(std::unique_ptr<_Ty> value) {
    ReferenceNode<_Ty> new_next;
    new_next.node = new Node<_Ty>;
    new_next.thread_ref_counter = 1;

    ReferenceNode<_Ty> old_tail = m_tail.load(std::memory_order_relaxed);
    for (;;) {
      old_tail = __increase_ref_rmw(m_tail, old_tail);

      void *old_data{nullptr};
      void *new_data = value.get();
      // Release publishes the constructed value through data. On failure,
      // old_data is used only to detect that another producer claimed this
      // slot, so the failed load does not need acquire ordering.
      //
      // Only nullptr is claimable. The terminal consumed_tag makes the state
      // transition one-way: nullptr -> value -> consumed_tag. Consequently, a
      // stale producer cannot resurrect an already retired node.
      if (old_tail.node->data.compare_exchange_strong(
              old_data, new_data, std::memory_order_release,
              std::memory_order_relaxed)) {

        ReferenceNode<_Ty> old_next{};
        // A failed CAS is a load and overwrites old_next. Because that returned
        // node is immediately republished through m_tail, failure must acquire
        // its publication. Success is acq_rel so failure-acquire is valid,
        // while the release half publishes new_next.
        if (!old_tail.node->next.compare_exchange_strong(
                old_next, new_next, std::memory_order_acq_rel,
                std::memory_order_acquire)) {
          delete new_next.node;
          new_next = old_next;
        }

        // data now owns the object; relinquish unique_ptr ownership without
        // destroying the published value.
        value.release();

        update_new_tail(old_tail, new_next);
        ++m_size;
        break;
      }

      ReferenceNode<_Ty> old_next{};
      // Same acquire-before-republish rule as the next CAS above.
      if (old_tail.node->next.compare_exchange_strong(
              old_next, new_next, std::memory_order_acq_rel,
              std::memory_order_acquire)) {
        /*new_tail*/ old_next = /*old_tail*/ new_next;
        new_next.node = new Node<_Ty>; // for next iteration!
      }
      update_new_tail(old_tail, old_next);
    }
  }

  [[nodiscard]]
  std::optional<std::unique_ptr<_Ty>> __pop() {
    ReferenceNode<_Ty> old_head = m_head.load(std::memory_order_relaxed);
    if (!old_head.node) {
      return std::nullopt;
    }

    for (;;) {
      old_head = __increase_ref_rmw(m_head, old_head);

      // Defensive invariant check only: after successful construction, a live
      // queue must never have a null head. Reaching this branch indicates an
      // invalid packed pointer or an already-corrupted object lifetime.
      if (!old_head.node) {
        __release_curr_thread_ref(old_head);
        return std::nullopt;
      }

      if (old_head.node == m_tail.load(std::memory_order_acquire).node) {
        rollback_or_release_head_ref(m_head, old_head);
        return std::nullopt;
      }

      // compare_exchange failure replaces old_head with the current head.
      // Preserve the node whose reference this iteration actually acquired;
      // otherwise a losing consumer decrements the new head instead of the old
      // one, corrupting its count and allowing a premature
      // delete/use-after-free.
      ReferenceNode<_Ty> backup = old_head;
      Node<_Ty> *ptr = backup.node;

      // next can be linked after ptr itself was published. Acquire next before
      // release-publishing it through m_head, preserving the transitive chain.
      auto next = ptr->next.load(std::memory_order_acquire);
      // Failure is relaxed because its replacement old_head is only fed back
      // into __increase_ref_rmw; that successful RMW acquires before use.
      if (m_head.compare_exchange_strong(old_head, next,
                                         std::memory_order_release,
                                         std::memory_order_relaxed)) {
        // Acquire consumes the producer's data-release before res is used. The
        // same atomic RMW installs the permanent consumed_tag; its write side
        // needs no release because other threads use the tag only as a state.
        void *res =
            ptr->data.exchange(consumed_ptr(), std::memory_order_acquire);
        // Transfer the winner's external references into the protected old
        // head. This call reclaims it only if its RMW owns the zero transition;
        // otherwise the last losing consumer performs reclamation later.
        __remove_node_from_heap(old_head);
        --m_size;
        return std::unique_ptr<_Ty>(static_cast<_Ty *>(res));
      }

      // Do not release old_head here: the failed CAS overwrote it with the new
      // head. Release ptr, the exact node protected above.
      ptr->release_curr_thread_ref();
    }
  }

private:
  static void *consumed_ptr() noexcept { return &consumed_tag; }

  // An empty pop has already added one external reference to protected_head.
  // While head still names that node, cancel the reference directly in the
  // packed external counter. A same-node CAS failure only means another thread
  // changed the aggregate count, so retry from the refreshed snapshot. If head
  // has moved, its winner's snapshot contains this unreverted reference; repay
  // it once through the original node's internal counter instead.
  static void
  rollback_or_release_head_ref(AtomicReferenceNode<_Ty> &head,
                               ReferenceNode<_Ty> protected_head) noexcept {
    // CAS failure overwrites current, so preserve the node actually protected
    // by this pop before entering the retry loop.
    Node<_Ty> *const ptr = protected_head.node;
    auto current = protected_head;

    for (;;) {
      if (current.node != ptr) {
        ptr->release_curr_thread_ref();
        return;
      }

      if (current.thread_ref_counter <= 1)
        std::terminate();

      auto desired = current;
      --desired.thread_ref_counter;

      if (head.compare_exchange_weak(current, desired,
                                     std::memory_order_relaxed,
                                     std::memory_order_relaxed)) {
        return;
      }
    }
  }

  void update_new_tail(ReferenceNode<_Ty> &old_tail,
                       const ReferenceNode<_Ty> &new_tail) {
    Node<_Ty> *backup = old_tail.node;
    while (!m_tail.compare_exchange_weak(old_tail, new_tail,
                                         std::memory_order_acq_rel,
                                         std::memory_order_acquire) &&
           backup == old_tail.node)
      ;

    if (backup == old_tail.node)
      __sync_threads_ref(old_tail);
    else
      backup->release_curr_thread_ref();
  }

  [[nodiscard]]
  static ReferenceNode<_Ty>
  __increase_ref_rmw(AtomicReferenceNode<_Ty> &main_node,
                     ReferenceNode<_Ty> &old) {
    // A successful CAS is an RMW in this atomic's modification order. Acquire
    // makes the node's publication visible before it is dereferenced; release
    // is conservative and is not required merely to extend a release sequence.
    // A failed CAS is only a load: it merely refreshes old for the next
    // attempt, whose eventual successful RMW performs the acquire, so failure
    // is relaxed.
    ReferenceNode<_Ty> new_ref;
    do {
      new_ref = old;
      // This packed field is bounded (17 bits with the current Apple layout,
      // 16 on x86-64). The empty-pop path now rolls its increment back while
      // head still names the same node, so sequential empty polling no longer
      // accumulates historical references. A capacity guard is still required:
      // enough simultaneously in-flight protectors could otherwise wrap the
      // field before any of them has a chance to release or roll back.
      new_ref.thread_ref_counter += 1;
    } while (!main_node.compare_exchange_weak(
        old, new_ref, std::memory_order_acq_rel, std::memory_order_relaxed));
    return new_ref;
  }

  static void __sync_threads_ref(ReferenceNode<_Ty> &old) {
    if (!old.node)
      return;
    const std::intptr_t any_other_threads = old.thread_ref_counter - 2;
    old.node->sync_threads_ref(any_other_threads);
  }

  static void __release_curr_thread_ref(ReferenceNode<_Ty> &old) {
    if (!old.node)
      return;
    old.node->release_curr_thread_ref();
  }

  static void __remove_node_from_heap(ReferenceNode<_Ty> &old) {
    __sync_threads_ref(old);
  }

private:
  std::atomic<std::size_t> m_size{};
  AtomicReferenceNode<_Ty> m_head;
  AtomicReferenceNode<_Ty> m_tail;
};

#endif // _QUEUE_LOCKFREE_HPP
