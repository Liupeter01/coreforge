#pragma once
#ifndef _HIERARCHICAL_LOCK_HPP_
#define _HIERARCHICAL_LOCK_HPP_
#include <mutex>
#include <stdexcept>

struct hierarchical_lock {
  explicit hierarchical_lock(std::size_t init_value)
      : m_current_level(init_value), m_pervious_level(0) {}
  hierarchical_lock(const hierarchical_lock &) = delete;
  hierarchical_lock &operator=(const hierarchical_lock &) = delete;

  void lock() {
    __level_check();
    m_mtx.lock();
    __update_level();
  }
  void unlock() {
    if (thread_level != m_current_level)
      throw std::runtime_error("Wrong Lock Detected!");

    thread_level = m_pervious_level; // recover
    m_mtx.unlock();
  }
  bool try_lock() {
    __level_check();
    if (!m_mtx.try_lock()) {
      return false;
    }
    __update_level();
    return true;
  }

private:
  void __level_check() const {
    if (thread_level <= m_current_level)
      throw std::runtime_error("Reverse Hierarchical Lock Detected!");
  }
  void __update_level() {
    m_pervious_level = thread_level;
    thread_level = m_current_level;
  }

private:
  std::mutex m_mtx; // mutex lock;
  std::size_t m_pervious_level;
  std::size_t m_current_level;
  static thread_local std::size_t thread_level; // level argument for this
                                                // thread
};

#endif
