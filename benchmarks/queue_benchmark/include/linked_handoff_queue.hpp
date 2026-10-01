#pragma once

#include "exponential_backoff.hpp"
#include "queue_latency_common.hpp"

#include <queue_lockfree.hpp>

namespace coreforge::queue_benchmark {

class LinkedHandoffQueue {
public:
  void push(const HandoffValue &value) { queue_.push(value); }

  [[nodiscard]] HandoffValue pop() {
    ExponentialBackoff backoff;
    for (;;) {
      auto value = queue_.pop();
      if (value.has_value())
        return **value;
      backoff.wait();
    }
  }

private:
  concurrency::ConcurrentQueue<HandoffValue> queue_;
};

} // namespace coreforge::queue_benchmark
