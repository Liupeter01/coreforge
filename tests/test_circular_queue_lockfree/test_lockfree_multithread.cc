#include <atomic>
#include <circular_queue_lockfree.hpp>
#include <gtest/gtest.h>
#include <string>
#include <thread>
#include <vector>

namespace {

struct ConcurrencyTestParam {
  std::size_t producer_count;
  std::size_t consumer_count;
  std::size_t total_values;
};

class LockFreeCircularQueueConcurrentTest
    : public ::testing::TestWithParam<ConcurrencyTestParam> {};

void run_concurrent_circular_queue_test(const ConcurrencyTestParam &param) {
  concurrency::ConcurrentCircularQueue<std::size_t, 2'097'152> queue;
  std::atomic<std::size_t> total_popped{0};
  std::atomic<std::size_t> producers_done{0};

  auto producer = [&](const std::size_t producer_index) {
    const std::size_t begin =
        param.total_values * producer_index / param.producer_count;
    const std::size_t end =
        param.total_values * (producer_index + 1) / param.producer_count;

    for (std::size_t value = begin; value < end; ++value) {
      // A bounded queue may be temporarily full; retrying preserves the test's
      // requested total instead of treating backpressure as a lost element.
      while (!queue.push(value))
        std::this_thread::yield();
    }

    producers_done.fetch_add(1, std::memory_order_release);
  };

  auto consumer = [&]() {
    for (;;) {
      std::size_t value{};
      if (queue.pop(value)) {
        total_popped.fetch_add(1, std::memory_order_relaxed);
        continue;
      }

      if (producers_done.load(std::memory_order_acquire) ==
          param.producer_count) {
        break;
      }

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

  EXPECT_EQ(total_popped.load(std::memory_order_relaxed), param.total_values);

  std::size_t value{};
  EXPECT_FALSE(queue.pop(value));
}

TEST_P(LockFreeCircularQueueConcurrentTest, PushAndPopAllValues) {
  const auto param = GetParam();
  ASSERT_GT(param.producer_count, 0U);
  ASSERT_GT(param.consumer_count, 0U);
  run_concurrent_circular_queue_test(param);
}

std::string concurrency_test_name(
    const ::testing::TestParamInfo<ConcurrencyTestParam> &info) {
  const auto &param = info.param;
  return "P" + std::to_string(param.producer_count) + "_C" +
         std::to_string(param.consumer_count) + "_Total" +
         std::to_string(param.total_values);
}

INSTANTIATE_TEST_SUITE_P(ConcurrencyConfigurations,
                         LockFreeCircularQueueConcurrentTest,
                         ::testing::ValuesIn(std::vector<ConcurrencyTestParam>{
                             {1, 1, 2'000'000},
                             {2, 4, 4'000'000},
                             {3, 2, 6'000'000},
                         }),
                         concurrency_test_name);

} // namespace
