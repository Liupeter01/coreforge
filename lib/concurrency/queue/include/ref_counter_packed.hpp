#pragma once
#ifndef _REF_COUNTER_PACKED_HPP_
#define _REF_COUNTER_PACKED_HPP_
#include <atomic>
#include <iostream>

namespace concurrency {
/*
 * [ word_bits - 1 : 2 ] thread_ref_counter
 * [             1 : 0 ] head_and_tail_ref_counter
 *
 * thread_ref_counter is a logical signed delta stored as an unsigned residue
 * in the upper field. Losing consumers may decrement it before the winner
 * transfers the external references, so a temporarily negative logical value
 * is part of the reclamation protocol rather than an underflow by itself.
 */
struct alignas(16) ref_counter_packed {

  using packed_t = std::uintptr_t;
  static constexpr int HEAD_TAIL_BITS = 2;
  static constexpr packed_t HEAD_TAIL_MASK = (1 << HEAD_TAIL_BITS) - 1;
  static constexpr int THREAD_REF_SHIFT = HEAD_TAIL_BITS;

  // Diagnostic accessor used for approximate observation or after workers have
  // joined. Relaxed is intentional: this value is not used to publish/consume
  // node data, and acquire would not turn a live sample into a stable snapshot.
  // The upper field is an unsigned modular encoding of a logical signed
  // balance; this truncating conversion is only relied on for the final zero
  // check, not for decoding an arbitrary in-flight negative balance.
  int get_threads_ref() const {
    return static_cast<int>(counter.load(std::memory_order_relaxed) >>
                            THREAD_REF_SHIFT);
  }

  int get_head_tail_ref() const {
    return static_cast<int>(counter.load(std::memory_order_relaxed) &
                            HEAD_TAIL_MASK);
  }

  void inc_thread_ref() {
    packed_t old_val = counter.load(std::memory_order_relaxed);
    packed_t new_val;
    do {
      std::intptr_t thread_ref =
          static_cast<std::intptr_t>(old_val >> THREAD_REF_SHIFT);
      std::intptr_t head_tail =
          static_cast<std::intptr_t>(old_val & HEAD_TAIL_MASK);

      thread_ref += 1;

      new_val = (static_cast<std::size_t>(thread_ref) << THREAD_REF_SHIFT) |
                (head_tail & HEAD_TAIL_MASK);
    } while (!counter.compare_exchange_weak(old_val, new_val,
                                            std::memory_order_relaxed));
  }

  bool dec_thread_ref() {
    packed_t old_val = counter.load(std::memory_order_relaxed);
    packed_t new_val;
    do {
      std::intptr_t thread_ref =
          static_cast<std::intptr_t>(old_val >> THREAD_REF_SHIFT);
      std::intptr_t head_tail =
          static_cast<std::intptr_t>(old_val & HEAD_TAIL_MASK);

      // A losing consumer may run before the winner transfers the external
      // references, so the logical thread_ref may temporarily be negative.
      thread_ref -= 1;

      // Convert before shifting so that temporary negative value is encoded
      // with defined unsigned modular arithmetic in the upper packed field.
      new_val = (static_cast<std::size_t>(thread_ref) << THREAD_REF_SHIFT) |
                (head_tail & HEAD_TAIL_MASK);
    } while (!counter.compare_exchange_weak(old_val, new_val,
                                            std::memory_order_acq_rel,
                                            std::memory_order_relaxed));

    return (0 == new_val);
  }

  // Legacy primitive retained for diagnostics/experimentation. Production
  // reclamation uses sync_threads_ref(), whose CAS both removes the structural
  // reference and uniquely reports whether the complete counter became zero.
  void dec_head_tail_ref() { counter.fetch_sub(1, std::memory_order_acq_rel); }

  bool sync_threads_ref(std::intptr_t any_other_threads) {
    packed_t old_val = counter.load(std::memory_order_relaxed);
    packed_t new_val;
    do {
      std::intptr_t thread_ref =
          static_cast<std::intptr_t>(old_val >> THREAD_REF_SHIFT);
      std::intptr_t head_tail =
          static_cast<std::intptr_t>(old_val & HEAD_TAIL_MASK);

      thread_ref += any_other_threads;
      head_tail -= 1;

      if (head_tail < 0) {
        throw std::runtime_error("head_tail underflow");
      }

      // Use the same unsigned modular encoding as dec_thread_ref(). The
      // winner's transferred external references cancel releases that losing
      // consumers may already have recorded against this node.
      new_val = (static_cast<std::size_t>(thread_ref) << THREAD_REF_SHIFT) |
                (head_tail & HEAD_TAIL_MASK);
    } while (!counter.compare_exchange_weak(old_val, new_val,
                                            std::memory_order_acq_rel,
                                            std::memory_order_relaxed));

    return (0 == new_val);
  }

  std::atomic<packed_t> counter{
      static_cast<packed_t>(0b10)}; // init head_tail = 2, thread_ref = 0
};
} // namespace concurrency

#endif // !_REF_COUNTER_PACKED_HPP_
