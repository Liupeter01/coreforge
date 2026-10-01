#pragma once

#include "exponential_backoff.hpp"
#include "queue_latency_common.hpp"

#include <circular_queue_lockfree.hpp>

namespace coreforge::queue_benchmark {

class CircularHandoffQueue {
public:
  void push(const HandoffValue &value) {
    ExponentialBackoff backoff;
    while (!queue_.push(value))
      backoff.wait();
  }

  [[nodiscard]] HandoffValue pop() {
    HandoffValue value;
    ExponentialBackoff backoff;
    while (!queue_.pop(value))
      backoff.wait();
    return value;
  }

private:
  concurrency::ConcurrentCircularQueue<HandoffValue, 65536> queue_;
};

} // namespace coreforge::queue_benchmark
