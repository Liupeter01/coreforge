#include <gtest/gtest.h>
#include <stack_lockfree.hpp>
#include <thread>

#define NUMBER 2000000

TEST(LockFreeStackTest, OneThreadForPushAndPop) {
  concurrency::ConcurrentStack<std::size_t> list;

  std::thread th1([&list]() {
    for (std::size_t i = 0; i < NUMBER; ++i)
      list.push(i);
  });
  std::thread th2([&list]() {
    std::size_t popped = 0;
    while (popped < NUMBER) {
      auto val = list.pop();
      if (val.has_value()) {
        ++popped;
        continue;
      }
      std::this_thread::yield();
    }
  });

  th1.join();
  th2.join();
  EXPECT_FALSE(list.pop().has_value());
  EXPECT_TRUE(list.empty());
}
