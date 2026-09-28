#include <atomic>
#include <chrono>
#include <gtest/gtest.h>
#include <queue_lockfree.hpp>
#include <string>
#include <thread>
#include <vector>

namespace {

struct ConcurrencyTestParam {
  std::size_t producer_count;
  std::size_t consumer_count;
  std::size_t total_values;
};

class LockFreeRefQueueConcurrentTest
    : public ::testing::TestWithParam<ConcurrencyTestParam> {};

constexpr std::size_t kRepeatCount = 10;

void run_concurrent_queue_test(const ConcurrencyTestParam &param) {
  concurrency::ConcurrentQueue<std::size_t> queue;
  std::atomic<std::size_t> total_popped{0};
  std::atomic<std::size_t> producers_done{0};

  std::vector<std::atomic<std::uint8_t>> seen(param.total_values);
  for (auto &slot : seen)
    slot.store(0, std::memory_order_relaxed);

  std::atomic<std::size_t> duplicate_count{0};
  std::atomic<std::size_t> out_of_range_count{0};

  auto producer = [&](const std::size_t producer_index) {
    const std::size_t begin =
        param.total_values * producer_index / param.producer_count;
    const std::size_t end =
        param.total_values * (producer_index + 1) / param.producer_count;

    for (std::size_t value = begin; value < end; ++value)
      queue.push(value);

    producers_done.fetch_add(1, std::memory_order_release);
  };

  // Intentionally do not serialize pop() with a mutex: doing so hides races in
  // the head-CAS failure path and in reference reclamation. producers_done is
  // only the shutdown signal; it does not protect the queue itself.
  auto consumer = [&]() {
    for (;;) {
      if (auto value = queue.pop(); value.has_value()) {
        total_popped.fetch_add(1, std::memory_order_relaxed);

        const std::size_t id = **value;
        if (id >= param.total_values) {
          out_of_range_count.fetch_add(1, std::memory_order_relaxed);
        } else if (seen[id].exchange(1, std::memory_order_relaxed) != 0) {
          duplicate_count.fetch_add(1, std::memory_order_relaxed);
        }
        continue;
      }

      if (producers_done.load(std::memory_order_acquire) ==
              param.producer_count &&
          queue.empty()) {
        break;
      }

      // Scheduling/backoff only; correctness must come from the queue atomics.
      std::this_thread::yield();
    }
  };

  std::vector<std::thread> producers;
  std::vector<std::thread> consumers;
  producers.reserve(param.producer_count);
  consumers.reserve(param.consumer_count);

  for (std::size_t i = 0; i < param.producer_count; ++i)
    producers.emplace_back(producer, i);
  for (std::size_t i = 0; i < param.consumer_count; ++i)
    consumers.emplace_back(consumer);

  for (auto &thread : producers)
    thread.join();
  for (auto &thread : consumers)
    thread.join();

  // A shared total detects a net loss or excess without turning it into the
  // hang caused by fixed per-consumer quotas. It does not prove exactly-once:
  // one duplicate can mask one lost value, so value-level tracking is still a
  // useful follow-up test.
  EXPECT_EQ(total_popped.load(std::memory_order_relaxed), param.total_values);
  EXPECT_TRUE(queue.empty());

  std::size_t missing_count = 0;
  for (const auto &slot : seen) {
    if (slot.load(std::memory_order_relaxed) == 0)
      ++missing_count;
  }

  EXPECT_EQ(out_of_range_count.load(std::memory_order_relaxed), 0U);
  EXPECT_EQ(duplicate_count.load(std::memory_order_relaxed), 0U);
  EXPECT_EQ(missing_count, 0U);
  EXPECT_EQ(total_popped.load(std::memory_order_relaxed), param.total_values);
}

TEST_P(LockFreeRefQueueConcurrentTest, PushAndPopAllValues) {
  const auto param = GetParam();
  ASSERT_GT(param.producer_count, 0U);
  ASSERT_GT(param.consumer_count, 0U);

  for (std::size_t iteration = 0; iteration < kRepeatCount; ++iteration) {
    run_concurrent_queue_test(param);
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
  }
}

std::string concurrency_test_name(
    const ::testing::TestParamInfo<ConcurrencyTestParam> &info) {
  const auto &param = info.param;
  return "P" + std::to_string(param.producer_count) + "_C" +
         std::to_string(param.consumer_count) + "_Total" +
         std::to_string(param.total_values);
}

INSTANTIATE_TEST_SUITE_P(ConcurrencyConfigurations,
                         LockFreeRefQueueConcurrentTest,
                         ::testing::ValuesIn(std::vector<ConcurrencyTestParam>{
                             {1, 1, 2'000'000},
                             {2, 4, 4'000'000},
                             {3, 2, 6'000'000},
                         }),
                         concurrency_test_name);

} // namespace
