#include <gtest/gtest.h>
#include <stack_lockfree.hpp>
#include <thread>

#define NUMBER 2000000

TEST(LockFreeStackTest, MultiThreadForPush3AndPop2) {
  concurrency::ConcurrentStack<std::size_t> list;
  auto producer = [&list]() {
    for (std::size_t i = 0; i < NUMBER; ++i)
      list.push(i);
  };

  std::vector<std::thread> producer_list;
  producer_list.emplace_back(producer);
  producer_list.emplace_back(producer);
  producer_list.emplace_back(producer);

  auto consumer = [&list, producer_number = producer_list.size()]() {
    std::size_t popped = 0;
    while (popped < NUMBER * producer_number / 2) {
      auto val = list.pop();
      if (val.has_value()) {
        ++popped;
        continue;
      }
      std::this_thread::sleep_for(std::chrono::microseconds(1));
    }
  };

  std::thread th4(consumer);
  std::thread th5(consumer);

  for (auto &thread : producer_list)
    thread.join();

  th4.join();
  th5.join();
  EXPECT_FALSE(list.pop().has_value());
  EXPECT_TRUE(list.empty());
}
