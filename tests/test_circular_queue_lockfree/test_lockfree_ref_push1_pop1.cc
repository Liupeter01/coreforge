#include <circular_queue_lockfree.hpp>
#include <gtest/gtest.h>
#include <thread>

#define NUMBER 2000000

TEST(ConcurrentCircularQueue, OneThreadForPushAndPop) {
  concurrency::ConcurrentCircularQueue<std::size_t, 2097152> queue;

  std::thread th1([&queue]() {
    for (std::size_t i = 0; i < NUMBER; ++i)
      queue.push(i);
  });
  std::thread th2([&queue]() {
    std::size_t popped = 0;
    while (popped < NUMBER) {
      std::size_t value;
      if (queue.pop(value)) {
        ++popped;
        continue;
      }
      std::this_thread::yield();
    }
  });

  th1.join();
  th2.join();

  std::size_t value;
  EXPECT_FALSE(queue.pop(value));
}
