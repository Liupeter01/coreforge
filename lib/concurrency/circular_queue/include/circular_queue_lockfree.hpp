#ifndef _CIRCULAR_QUEUE_LOCKFREE_HPP_
#define _CIRCULAR_QUEUE_LOCKFREE_HPP_
#include <atomic>
#include <iostream>
#include <memory>

namespace concurrency {
template <typename _Ty, std::size_t _Size> class ConcurrentCircularQueue;
}

template <typename _Ty, std::size_t _Size>
class concurrency::ConcurrentCircularQueue : private std::allocator<_Ty> {
  ConcurrentCircularQueue(const ConcurrentCircularQueue &) = delete;
  ConcurrentCircularQueue &operator=(const ConcurrentCircularQueue &) = delete;

public:
  ConcurrentCircularQueue()
      : m_max_size(_Size + 1),
        m_data(std::allocator<_Ty>::allocate(m_max_size)), m_tail(0),
        m_updated_tail(0), m_head(0) {}
  virtual ~ConcurrentCircularQueue() {
    for (std::size_t index = m_head; index != m_tail; index = next(index))
      std::allocator<_Ty>::destroy(m_data + index); //_Ty's dtor
    std::allocator<_Ty>::deallocate(m_data, m_max_size);
  }

public:
  bool push(const _Ty &value) {
    std::size_t tail_value{};
    do {
      tail_value = m_tail.load(std::memory_order_relaxed);
      if (next(tail_value) == m_head.load(std::memory_order_acquire))
        return false;
    } while (!m_tail.compare_exchange_weak(tail_value, next(tail_value),
                                           std::memory_order_release,
                                           std::memory_order_relaxed));

    m_data[tail_value] = value;

    std::size_t update_tail;
    do {
      update_tail = tail_value;
    } while (!m_updated_tail.compare_exchange_weak(
        update_tail, next(update_tail), std::memory_order_release,
        std::memory_order_relaxed));

    return true;
  }

  bool pop(_Ty &value) {
    std::size_t head_value{};
    do {
      head_value = m_head.load(std::memory_order_relaxed);
      if (head_value == m_tail.load(std::memory_order_acquire))
        return false;

      if (head_value == m_updated_tail.load(std::memory_order_acquire))
        return false;

      value = m_data[head_value];
    } while (!m_head.compare_exchange_weak(head_value, next(head_value),
                                           std::memory_order_release,
                                           std::memory_order_relaxed));

    return true;
  }

protected:
  std::size_t next(const std::size_t index) const {
    return (index + 1) % m_max_size;
  }

private:
  const std::size_t m_max_size;
  _Ty *m_data;

  std::atomic<std::size_t> m_head;
  std::atomic<std::size_t> m_tail;

  std::atomic<std::size_t> m_updated_tail;
};

#endif // _CIRCULAR_QUEUE_LOCKFREE_HPP_
