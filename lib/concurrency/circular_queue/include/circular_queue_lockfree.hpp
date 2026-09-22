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
                              Behind = -1,		//sequence acquire from cell < current(old) tail value
                              Equal = 0,			//sequence acquire from cell == current(old) tail value
                              Ahead = 1				//sequence acquire from cell > current(old) tail value
                    };

                    struct alignas(64) CacheLineAlignedAtomic {
                              std::atomic<std::size_t> atomic_{};
                    };

                    [[nodiscard]]
                    constexpr inline GenerationOrder
                              compare_generation(const std::size_t seq,
                                        const  std::size_t expected) noexcept {

                              using U = std::size_t;
                              static_assert(std::is_unsigned_v<U>);

                              constexpr auto digits = std::numeric_limits<U>::digits;
                              constexpr U high_bit = U{ 1 } << (digits - 1);

                              const U diff = (seq - expected);

                              if (diff == 0)
                                        return GenerationOrder::Equal;

                              if ((diff & high_bit) != 0)
                                        return GenerationOrder::Behind;

                              return GenerationOrder::Ahead;
                    }

                    template<typename _Ty>
                    struct Cell {
                              inline Cell& operator=(const Cell& o) {
                                        if (&o == this) return *this;
                                        this->data_ = o.data_;
                                        this->sequence_.store(o.sequence_.load(std::memory_order_acquire), std::memory_order_release);
                                        return *this;
                              }
                              std::atomic<std::size_t> sequence_{ 0 };
                              _Ty data_{};
                    };
          }

                              //bool push(const _Ty& value) {
                    //          std::size_t tail_value = m_tail.atomic_.load(std::memory_order_relaxed);
                    //          do {
                    //                    if (next(tail_value) == m_head.atomic_.load(std::memory_order_acquire))
                    //                              return false;

                    //          } while (!m_tail.atomic_.compare_exchange_weak(tail_value, next(tail_value),
                    //                    std::memory_order_acq_rel,
                    //                    std::memory_order_relaxed));

                    //          m_data[tail_value] = value;

                    //          std::size_t update_tail;
                    //          do {
                    //                    update_tail = tail_value;
                    //          } while (!m_updated_tail.atomic_.compare_exchange_weak(
                    //                    update_tail, next(update_tail),
                    //                    std::memory_order_acq_rel,
                    //                    std::memory_order_relaxed));

                    //          return true;
                    //}

                    //bool pop(_Ty& value) {
                    //          std::size_t head_value = m_head.atomic_.load(std::memory_order_relaxed);

                    //          do {
                    //                    if (head_value == m_tail.atomic_.load(std::memory_order_acquire))
                    //                              return false;

                    //                    if (head_value == m_updated_tail.atomic_.load(std::memory_order_acquire))
                    //                              return false;

                    //                    value = m_data[head_value];

                    //          } while (!m_head.atomic_.compare_exchange_weak(head_value, next(head_value),
                    //                    std::memory_order_acq_rel,
                    //                    std::memory_order_relaxed));

                    //          return true;
                    //}

          template <typename _Ty, std::size_t _Size>
          class ConcurrentCircularQueue  {
                    ConcurrentCircularQueue(const ConcurrentCircularQueue&) = delete;
                    ConcurrentCircularQueue& operator=(const ConcurrentCircularQueue&) = delete;

                    static_assert(_Size > 0 && _Size < std::numeric_limits<std::size_t>::max(), "_Size must >0 and < std::size_t max");
                    static_assert((_Size & (_Size  - 1)) == 0, "_Size must be power of two");

          public:
                    ~ConcurrentCircularQueue() = default;
                    inline  ConcurrentCircularQueue(): m_slots(_Size){
                              m_tail.atomic_.store(0, std::memory_order_relaxed);
                              m_head.atomic_.store(0, std::memory_order_relaxed);

                              for (std::size_t i = 0; i < _Size; ++i) {
                                        m_slots[i].sequence_.store(i, std::memory_order_release);
                              }
                    }

          public:
                    inline bool push(const _Ty& value){

                              std::size_t logical_pos =  m_tail.atomic_.load(std::memory_order_relaxed);
                              details::Cell<_Ty>* cell{};

                              for (;;) {

                                        if (logical_pos + 1 == m_head.atomic_.load(std::memory_order_acquire))
                                                  return false;

                                       cell = &m_slots[logical_pos & (_Size - 1)];
                                        std::size_t sequence = cell->sequence_.load(std::memory_order_acquire);
                                        
                                        const details::GenerationOrder order = details::compare_generation(sequence, logical_pos);
                                        if (order == details::GenerationOrder::Equal) {
                                                  if (m_tail.atomic_.compare_exchange_weak(logical_pos, logical_pos + 1,
                                                            std::memory_order_acq_rel, 
                                                            std::memory_order_relaxed)) {
                                                            break;
                                                  }
                                        }
                                        else if(order == details::GenerationOrder::Ahead){
                                                  logical_pos = m_tail.atomic_.load(std::memory_order_relaxed);
                                                  continue;
                                        }
                                        else {
                                                  return false;
                                        }
                              }

                              cell->data_ = value;
                              cell->sequence_.store(logical_pos + 1, std::memory_order_release);
                              return true;
                    }

                    inline bool pop(_Ty& value) {
                              std::size_t logical_pos = m_head.atomic_.load(std::memory_order_relaxed);
                              details::Cell<_Ty>* cell{};

                              for (;;) {

                                        if (logical_pos == m_tail.atomic_.load(std::memory_order_acquire))
                                                  return false;

                                        cell = &m_slots[logical_pos & (_Size - 1)];
                                        std::size_t sequence = cell->sequence_.load(std::memory_order_acquire);

                                        const details::GenerationOrder order = details::compare_generation(sequence, logical_pos + 1);

                                        if (order == details::GenerationOrder::Equal) {
                                                  if (m_head.atomic_.compare_exchange_weak(logical_pos, logical_pos + 1,
                                                            std::memory_order_acq_rel,
                                                            std::memory_order_relaxed)) {
                                                            break;
                                                  }
                                        }
                                        else if (order == details::GenerationOrder::Ahead) {
                                                  logical_pos = m_head.atomic_.load(std::memory_order_relaxed);
                                                  continue;
                                        }
                                        else {
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
}

#endif //_CIRCULAR_QUEUE_LOCKFREE_HPP_