#ifndef _CIRCULAR_QUEUE_LOCKFREE_HPP_
#define _CIRCULAR_QUEUE_LOCKFREE_HPP_
#include <atomic>
#include <iostream>
#include <memory>
#include <mutex>
#include <vector>

namespace concurrency {

namespace details {
enum class GenerationOrder {
  Behind = -1, // sequence acquire from cell < current(old) tail value
  Equal = 0,   // sequence acquire from cell == current(old) tail value
  Ahead = 1    // sequence acquire from cell > current(old) tail value
};

struct alignas(64) CacheLineAlignedAtomic {
  std::atomic<std::size_t> atomic_{};
};

[[nodiscard]]
constexpr inline GenerationOrder
compare_generation(const std::size_t seq, const std::size_t expected) noexcept {

  using U = std::size_t;
  static_assert(std::is_unsigned_v<U>);

  constexpr auto digits = std::numeric_limits<U>::digits;
  constexpr U high_bit = U{1} << (digits - 1);

  const U diff = (seq - expected);

  if (diff == 0)
    return GenerationOrder::Equal;

  if ((diff & high_bit) != 0)
    return GenerationOrder::Behind;

  return GenerationOrder::Ahead;
}

template <typename _Ty> struct Cell {
  inline Cell &operator=(const Cell &o) {
    if (&o == this)
      return *this;
    this->data_ = o.data_;
    this->sequence_.store(o.sequence_.load(std::memory_order_acquire),
                          std::memory_order_release);
    return *this;
  }
  std::atomic<std::size_t> sequence_{0};
  _Ty data_{};
};
} // namespace details

// Why the previous implementation below is not safe:
//
// push() reserves a position by advancing m_tail before it writes the value.
// m_updated_tail then forces producers to publish those writes in reservation
// order. If a producer is suspended after advancing m_tail, every later
// producer spins behind it, so the global publication frontier serializes the
// producers and the algorithm does not provide lock-free progress.
//
// The more serious problem is in pop(): it copies m_data[head_value] before it
// has successfully claimed head_value with compare_exchange_weak(). Multiple
// consumers may therefore start reading the same non-atomic object. After one
// consumer wins the CAS, a producer may observe the advanced head and reuse
// that slot while a losing consumer is still copying from it. That creates an
// unsynchronized read/write data race (and undefined behavior for a non-atomic
// _Ty). A successful CAS only grants ownership of the head counter; it cannot
// retroactively make the earlier data read safe.
//
// The current implementation fixes both problems with a sequence number per
// cell: a consumer reads data only after claiming the cell, and the cell is not
// made reusable until that read has completed.
//
template <typename _Ty, std::size_t _Size> class ConcurrentCircularQueue {
  ConcurrentCircularQueue(const ConcurrentCircularQueue &) = delete;
  ConcurrentCircularQueue &operator=(const ConcurrentCircularQueue &) = delete;

  // With one cell, the published and next-free generations overlap, so a
  // second push can be accepted before the first value is consumed.
  static_assert(_Size > 1 && _Size < std::numeric_limits<std::size_t>::max(),
                "_Size must be greater than 1 and less than std::size_t max");
  static_assert((_Size & (_Size - 1)) == 0, "_Size must be power of two");

public:
  ~ConcurrentCircularQueue() = default;
  inline ConcurrentCircularQueue() : m_slots(_Size) {
    m_tail.atomic_.store(0, std::memory_order_relaxed);
    m_head.atomic_.store(0, std::memory_order_relaxed);

    for (std::size_t i = 0; i < _Size; ++i) {
      m_slots[i].sequence_.store(i, std::memory_order_release);
    }
  }

public:
  inline bool push(const _Ty &value) {

    std::size_t logical_pos = m_tail.atomic_.load(std::memory_order_relaxed);
    details::Cell<_Ty> *cell{};

    for (;;) {

      if (logical_pos + 1 == m_head.atomic_.load(std::memory_order_acquire))
        return false;

      cell = &m_slots[logical_pos & (_Size - 1)];
      std::size_t sequence = cell->sequence_.load(std::memory_order_acquire);

      const details::GenerationOrder order =
          details::compare_generation(sequence, logical_pos);
      if (order == details::GenerationOrder::Equal) {
        if (m_tail.atomic_.compare_exchange_weak(logical_pos, logical_pos + 1,
                                                 std::memory_order_acq_rel,
                                                 std::memory_order_relaxed)) {
          break;
        }
      } else if (order == details::GenerationOrder::Ahead) {
        logical_pos = m_tail.atomic_.load(std::memory_order_relaxed);
        continue;
      } else {
        return false;
      }
    }

    cell->data_ = value;
    cell->sequence_.store(logical_pos + 1, std::memory_order_release);
    return true;
  }

  inline bool pop(_Ty &value) {
    std::size_t logical_pos = m_head.atomic_.load(std::memory_order_relaxed);
    details::Cell<_Ty> *cell{};

    for (;;) {

      if (logical_pos == m_tail.atomic_.load(std::memory_order_acquire))
        return false;

      cell = &m_slots[logical_pos & (_Size - 1)];
      std::size_t sequence = cell->sequence_.load(std::memory_order_acquire);

      const details::GenerationOrder order =
          details::compare_generation(sequence, logical_pos + 1);

      if (order == details::GenerationOrder::Equal) {
        if (m_head.atomic_.compare_exchange_weak(logical_pos, logical_pos + 1,
                                                 std::memory_order_acq_rel,
                                                 std::memory_order_relaxed)) {
          break;
        }
      } else if (order == details::GenerationOrder::Ahead) {
        logical_pos = m_head.atomic_.load(std::memory_order_relaxed);
        continue;
      } else {
        return false;
      }
    }

    value = cell->data_;
    cell->sequence_.store(logical_pos + _Size, std::memory_order_release);
    return true;
  }

private:
  std::vector<details::Cell<_Ty>> m_slots;

  details::CacheLineAlignedAtomic m_head;
  details::CacheLineAlignedAtomic m_tail;
};
} // namespace concurrency

#endif //_CIRCULAR_QUEUE_LOCKFREE_HPP_
