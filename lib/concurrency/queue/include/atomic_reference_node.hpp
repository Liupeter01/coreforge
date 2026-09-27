#pragma once
#ifndef _ATOMIC_REFERENCE_NODE_HPP_
#define _ATOMIC_REFERENCE_NODE_HPP_
#include <cassert>
#include <iostream>
#include <ref_counter_packed.hpp>

namespace concurrency {

template <typename _Ty> struct ReferenceNode;

template <typename _Ty> struct AtomicReferenceNode;

template <typename _Ty> struct Node {
  Node() : inner_ref_counter{} {

    ReferenceNode<_Ty> node;
    node.node = nullptr;
    node.thread_ref_counter = 0;

    next.store(node, std::memory_order_release);
    data.store(nullptr, std::memory_order_release);
  }

  // Transferring the external references is an acq_rel RMW. Its acquire half
  // joins the preceding release/RMW chain on this counter; if it writes the
  // complete zero value, this thread uniquely owns reclamation.
  void sync_threads_ref(const std::intptr_t any_other_threads) {
    if (inner_ref_counter.sync_threads_ref(any_other_threads)) {
      delete this;
    }
  }

  // This acq_rel RMW likewise joins the counter's synchronization chain. Only
  // the RMW that changes the complete packed count to zero may reclaim the
  // node, preventing both missed deletion and a competing double delete.
  void release_curr_thread_ref() {
    if (inner_ref_counter.dec_thread_ref()) {
      delete this;
    }
  }

  AtomicReferenceNode<_Ty> next;
  std::atomic<void *> data;
  ref_counter_packed inner_ref_counter;
};

// Non-atomic snapshot type. Access the packed shared state only through
// AtomicReferenceNode; this structure itself provides no synchronization.
template <typename _Ty> struct alignas(16) ReferenceNode {
  std::intptr_t
      thread_ref_counter; // how many threads are referencing this node?
  Node<_Ty> *node;
};
} // namespace concurrency

namespace concurrency {

/*
 * [ word_bits - 1 : PTR_BITS ] external thread_ref_counter
 * [       PTR_BITS - 1 : 0 ]   pointer
 */
// This is 16-byte type alignment for the packed atomic; it is not cache-line
// isolation between the queue's head and tail.
template <typename _Ty> struct alignas(16) AtomicReferenceNode {

  using packed_t = std::uintptr_t;

  static_assert(sizeof(packed_t) == 8,
              "AtomicReferenceNode requires 64-bit uintptr_t");

  // Platform-specific detection
#if defined(__x86_64__) || defined(_M_X64)
  static constexpr int PTR_BITS =
      48; // x86_64 canonical address (Linux, macOS, Windows)

#elif defined(__aarch64__) || defined(_M_ARM64)
  static constexpr bool IS_64BIT = true;

#if defined(__APPLE__)
  static constexpr int PTR_BITS =
      47; // Apple M1/M2 uses 47-bit VAs with top byte ignored/PAC
#else
  static constexpr int PTR_BITS = 48; // Default ARM64 Linux (e.g. Raspberry Pi)
#endif

#else
#error "AtomicReferenceNode requires a 64-bit x86-64 or AArch64 target"
#endif

  using reference_node = ReferenceNode<_Ty>;
  using node_pointer = Node<_Ty> *;
  static constexpr packed_t PTR_MASK = (1ULL << PTR_BITS) - 1;
  static constexpr int REF_SHIFT = PTR_BITS;

  static packed_t pack(node_pointer node, std::intptr_t ref) {
    // TODO(portability): verify that every supported pointer representation
    // fits PTR_MASK; masking an out-of-range address would silently corrupt it.
    return (reinterpret_cast<packed_t>(node) & PTR_MASK) |
           (static_cast<packed_t>(ref) << REF_SHIFT);
  }

  static node_pointer extract_ptr(packed_t val) {
    return reinterpret_cast<node_pointer>(val & PTR_MASK);
  }

  static std::intptr_t extract_ref(packed_t val) {
    // This field is a nonnegative external reference count and is deliberately
    // zero-extended. It must not wrap past the bits available above PTR_BITS.
    return static_cast<std::intptr_t>(val >> REF_SHIFT);
  }

  // Atomic load into structured ReferenceNode
  reference_node
  load(std::memory_order order = std::memory_order_acquire) const {
    packed_t val = counter.load(order);
    return {extract_ref(val), extract_ptr(val)};
  }

  // Atomic store from structured ReferenceNode
  void store(const reference_node &rn,
             std::memory_order order = std::memory_order_release) {
    counter.store(pack(rn.node, rn.thread_ref_counter), order);
  }

  // Helpers for direct reference counter manipulation
  void inc_ref(std::memory_order order = std::memory_order_acq_rel) {
    counter.fetch_add(static_cast<packed_t>(1) << REF_SHIFT, order);
  }

  void dec_ref(std::memory_order order = std::memory_order_acq_rel) {
    counter.fetch_sub(static_cast<packed_t>(1) << REF_SHIFT, order);
  }

  // Compare-and-swap with structured reference_node. The default acq_rel
  // success order permits acquire on failure; performance-sensitive callers
  // may still provide weaker valid orders when they do not consume a value.
  bool
  compare_exchange_weak(reference_node &expected, const reference_node &desired,
                        std::memory_order success = std::memory_order_acq_rel,
                        std::memory_order fail = std::memory_order_acquire) {
    packed_t expected_raw = pack(expected.node, expected.thread_ref_counter);
    packed_t desired_raw = pack(desired.node, desired.thread_ref_counter);
    bool ok =
        counter.compare_exchange_weak(expected_raw, desired_raw, success, fail);
    if (!ok) {
      expected = {extract_ref(expected_raw), extract_ptr(expected_raw)};
    }
    return ok;
  }

  bool
  compare_exchange_strong(reference_node &expected,
                          const reference_node &desired,
                          std::memory_order success = std::memory_order_acq_rel,
                          std::memory_order fail = std::memory_order_acquire) {
    packed_t expected_raw = pack(expected.node, expected.thread_ref_counter);
    packed_t desired_raw = pack(desired.node, desired.thread_ref_counter);
    bool ok = counter.compare_exchange_strong(expected_raw, desired_raw,
                                              success, fail);
    if (!ok) {
      expected = {extract_ref(expected_raw), extract_ptr(expected_raw)};
    }
    return ok;
  }

  std::atomic<packed_t> counter{static_cast<packed_t>(
      0)}; // init thread_ref_counter = 0  node = nullptr(0)
};
} // namespace concurrency

#endif // !_ATOMIC_REFERENCE_NODE_HPP_
