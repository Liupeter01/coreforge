#include <gtest/gtest.h>
#include <circular_queue_lockfree.hpp>
#include <thread>

#define NUMBER 2000000

TEST(ConcurrentCircularQueue, MultiThreadForPush3AndPop2) {
          concurrency::ConcurrentCircularQueue<std::size_t, 2097152> queue;

  auto producer = [&queue]() {
    for (std::size_t i = 0; i < NUMBER; ++i)
      queue.push(i);
  };

  std::vector<std::thread> producer_list;
  producer_list.emplace_back(producer);
  producer_list.emplace_back(producer);
  producer_list.emplace_back(producer);

  auto consumer = [&queue, producer_number = producer_list.size()]() {
    std::size_t popped = 0;
    while (popped < NUMBER * producer_number / 2) {
              std::size_t value;
      if (queue.pop(value)) {
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

  std::size_t value;
  EXPECT_FALSE(queue.pop(value));
}
