#ifndef _CIRCULAR_QUEUE_LK_HPP_
#define _CIRCULAR_QUEUE_LK_HPP_
#include <iostream>
#include <memory>
#include <mutex>

template <typename _Ty, std::size_t _Size>
class CircularQueueLk : private std::allocator<_Ty> {
  CircularQueueLk(const CircularQueueLk &) = delete;
  CircularQueueLk &operator=(const CircularQueueLk &) = delete;

  static_assert(_Size > 0 && _Size < std::numeric_limits<std::size_t>::max(),
                "_Size must >0 and < std::size_t max");

public:
  CircularQueueLk()
      : m_max_size(_Size + 1),
        m_data(std::allocator<_Ty>::allocate(m_max_size)), m_tail(0),
        m_head(0) {}
  virtual ~CircularQueueLk() {
    std::lock_guard<std::mutex> _lckg(m_mtx);
    for (std::size_t index = m_head; index != m_tail; index = next(index))
      std::allocator<_Ty>::destroy(m_data + index); //_Ty's dtor
    std::allocator<_Ty>::deallocate(m_data, m_max_size);
  }

public:
  bool empty() const { return m_tail == m_head; }

  bool isfull() const { return next(m_tail) == m_head; }

  bool push(const _Ty &value) { return emplace(value); }
  bool push(_Ty &&value) { return emplace(std::move(value)); }
  bool pop(_Ty &value) {
    std::lock_guard<std::mutex> _lckg(m_mtx);
    if (empty())
      return false;

    if (m_data + m_head == nullptr) {
      return false;
    }
    value = std::move(m_data[m_head]);
    m_head = next(m_head);
    return true;
  }

protected:
  std::size_t next(const std::size_t index) const {
    return (index + 1) % m_max_size;
  }

  template <typename... Args> bool emplace(Args &&...args) {
    std::lock_guard<std::mutex> _lckg(m_mtx);
    if (isfull())
      return false;
    std::allocator<_Ty>::construct(m_data + m_tail,
                                   std::forward<Args>(args)...);
    m_tail = next(m_tail);
    return true;
  }

private:
  std::mutex m_mtx;
  std::size_t m_head = 0;
  std::size_t m_tail = 0;
  const std::size_t m_max_size;
  _Ty *m_data;
};

#endif // _CIRCULAR_QUEUE_LK_HPP_
