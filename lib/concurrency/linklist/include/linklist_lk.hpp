#pragma once
#ifndef _LINKLIST_LK_HPP
#define _LINKLIST_LK_HPP
#include <atomic>
#include <condition_variable>
#include <memory>
#include <mutex>
#include <optional>

namespace concurrency {
template <typename _Ty> class LinkListLK;
}

template <typename _Ty> class concurrency::LinkListLK {
  struct Node {
    Node() : next(nullptr), data(nullptr) {}
    Node(const _Ty &value)
        : next(nullptr), data(std::make_shared<_Ty>(value)) {}
    std::mutex mutex;
    std::unique_ptr<Node> next;
    std::shared_ptr<_Ty> data;
  };

  LinkListLK(const LinkListLK &) = delete;
  LinkListLK &operator=(const LinkListLK &) = delete;
  LinkListLK(LinkListLK &&) = delete;
  LinkListLK &operator=(LinkListLK &&) = delete;

public:
  LinkListLK() : m_size(0), m_head(std::make_unique<Node>()) {
    m_tail = m_head.get();
  }
  virtual ~LinkListLK() {
    remove_if([](const auto &value) { return true; });
  }

public:
  void push_front(const _Ty &value) {
    ++m_size;
    auto new_node = std::make_unique<Node>(value);
    Node *ptr = new_node.get();
    std::lock_guard<std::mutex> _lckg(m_head->mutex);
    new_node->next = std::move(m_head->next);
    Node *isFirstNode = new_node->next.get();
    m_head->next = std::move(new_node);

    /*This is the first node!*/
    if (isFirstNode == nullptr) {
      std::lock_guard<std::mutex> _lckg_tail(m_tail_mtx);
      m_tail = ptr;
    }
  }

  void push_back(const _Ty &value) {
    ++m_size;
    auto new_node = std::make_unique<Node>(value);
    Node *ptr = new_node.get();
    std::lock_guard<std::mutex> _lckg_tail(m_tail_mtx);
    {
      std::lock_guard<std::mutex> _lckg_node(m_tail->mutex);
      m_tail->next = std::move(new_node);
    }
    m_tail = ptr;
  }
  void push_back(_Ty &&value) {
    ++m_size;
    auto new_node = std::make_unique<Node>(std::move(value));
    Node *ptr = new_node.get();

    std::lock(m_tail_mtx, m_tail->mutex);
    std::unique_lock<std::mutex> _lckg_tail(m_tail_mtx, std::adopt_lock);
    std::unique_lock<std::mutex> _lckg_node(m_tail->mutex, std::adopt_lock);

    m_tail->next = std::move(new_node);
    m_tail = ptr;
  }

  std::size_t size() const { return m_size; }
  template <typename _Pred> void remove_if(_Pred &&pred) {

    Node *current = m_head.get(); // currently, head node;
    std::unique_lock<std::mutex> _prev_lckg(m_head->mutex);
    while (Node *next = current->next.get()) {
      std::unique_lock<std::mutex> _cur_lckg(next->mutex);
      // Satisfied! Under Lock Condition
      if (pred(*next->data)) {
        auto __ext_life_length = std::move(
            current
                ->next); // no longer need this, but wee need it's life length!
        current->next = std::move(next->next);

        if (current->next.get() == nullptr) {
          std::lock_guard<std::mutex> _lckg_tail(m_tail_mtx);
          if (m_tail == next)
            m_tail = current;
        }
        _cur_lckg.unlock();
        --m_size;
        continue;
      }
      current = next;
      _prev_lckg.unlock(); // have already modify, no longer needs to lock
      _prev_lckg = std::move(_cur_lckg);
    }
  }

  template <typename _Pred> void for_each(_Pred &&pred) {

    Node *current = m_head.get(); // currently, head node;
    std::unique_lock<std::mutex> _prev_lckg(m_head->mutex);
    while (Node *next = current->next.get()) {

      std::unique_lock<std::mutex> _cur_lckg(next->mutex);

      _prev_lckg.unlock();
      pred(*next->data);
      current = next;
      _prev_lckg = std::move(_cur_lckg);
    }
  }

  template <typename _Pred>
  std::optional<std::shared_ptr<_Ty>> find_first_if(_Pred &&pred) {
    Node *current = m_head.get(); // currently, head node;
    std::unique_lock<std::mutex> _prev_lckg(m_head->mutex);
    while (Node *next = current->next.get()) {
      std::unique_lock<std::mutex> _cur_lckg(next->mutex);
      _prev_lckg.unlock();

      if (pred(*next->data)) {
        return next->data;
      }
      current = next;
      _prev_lckg = std::move(_cur_lckg);
    }
    return std::nullopt;
  }

private:
  std::atomic<std::size_t> m_size{};
  std::unique_ptr<Node> m_head;
  Node *m_tail;
  std::mutex m_tail_mtx;
};

#endif //_LINKLIST_LK_HPP
