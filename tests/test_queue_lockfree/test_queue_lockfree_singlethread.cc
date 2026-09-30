#include <atomic>
#include <cassert>
#include <cstdint>
#include <gtest/gtest.h>
#include <iostream>
#include <memory>
#include <optional>
#include <stdexcept>
#include <type_traits>

// White-box access is limited to the packed-counter regression below. Include
// every standard-library dependency first so this macro cannot rewrite access
// specifiers inside a standard header.
#define private public
#include <queue_lockfree.hpp>
#undef private

struct MyClass {
  int a;
};

#define TESTCOUNT 50000

TEST(LockFreeRefQueueTest, SingleThreadReferenceCounterTest) {
  concurrency::ConcurrentQueue<MyClass> queue;

  MyClass a;
  a.a = 100;

  for (std::size_t i = 0; i < TESTCOUNT; ++i) {
    EXPECT_TRUE(queue.empty());
    queue.pop();
    queue.pop();
    queue.pop();

    EXPECT_TRUE(queue.empty());
    queue.push(a);

    EXPECT_FALSE(queue.empty());
    queue.pop();
  }

  EXPECT_TRUE(queue.empty());
}

TEST(LockFreeRefQueueTest, SingleThreadCompleteTest) {
  concurrency::ConcurrentQueue<MyClass> queue;

  MyClass a;
  a.a = 100;

  for (std::size_t i = 0; i < TESTCOUNT; ++i) {
    EXPECT_TRUE(queue.empty());
    queue.push(a);
    queue.push(a);
    EXPECT_FALSE(queue.empty());
    queue.push(a);

    queue.pop();
    queue.pop();
    EXPECT_FALSE(queue.empty());
    queue.pop();

    EXPECT_TRUE(queue.empty());
  }
}

TEST(LockFreeRefQueueTest, SingleThreadEmptyTest) {
  concurrency::ConcurrentQueue<MyClass> queue;

  MyClass a;
  a.a = 100;

  for (std::size_t i = 0; i < TESTCOUNT; ++i) {
    EXPECT_FALSE(queue.pop().has_value());
    EXPECT_TRUE(queue.empty());

    queue.push(a);
    queue.push(a);
    queue.push(a);

    EXPECT_TRUE(queue.pop().has_value());
    EXPECT_TRUE(queue.pop().has_value());
    EXPECT_TRUE(queue.pop().has_value());
    EXPECT_FALSE(queue.pop().has_value());
  }
  EXPECT_TRUE(queue.empty());
}

TEST(LockFreeRefQueueTest, SingleThreadCorrectValueTest) {
  concurrency::ConcurrentQueue<int> queue;

  for (std::size_t i = 0; i < TESTCOUNT; ++i) {
    EXPECT_FALSE(queue.pop().has_value());
    EXPECT_TRUE(queue.empty());

    queue.push(i);

    auto opt = queue.pop();
    EXPECT_TRUE(opt.has_value());
    EXPECT_EQ(*opt.value(), i);

    EXPECT_TRUE(queue.empty());
    EXPECT_FALSE(queue.pop().has_value());
  }
  EXPECT_TRUE(queue.empty());
}

TEST(LockFreeRefQueueTest,
     EmptyPopRollbackPreventsExternalCounterWrapAfterQueueDrains) {
  using Queue = concurrency::ConcurrentQueue<int>;
  using AtomicRef = concurrency::AtomicReferenceNode<int>;

  Queue queue;

  // Move head through several real nodes before stressing the final empty
  // dummy. This catches implementations that only behave correctly for the
  // constructor's initial node.
  constexpr int kValueCount = 5;
  for (int value = 0; value < kValueCount; ++value)
    queue.push(value);

  for (int expected = 0; expected < kValueCount; ++expected) {
    auto value = queue.pop();
    ASSERT_TRUE(value.has_value());
    EXPECT_EQ(**value, expected);
  }
  ASSERT_TRUE(queue.empty());

  const auto drained_head = queue.m_head.load(std::memory_order_relaxed);
  ASSERT_NE(drained_head.node, nullptr);
  ASSERT_EQ(drained_head.thread_ref_counter, 1);

  constexpr std::size_t kReferenceBits =
      sizeof(std::uintptr_t) * 8 - AtomicRef::PTR_BITS;
  static_assert(kReferenceBits < sizeof(std::size_t) * 8);
  constexpr std::size_t kExternalModulus = std::size_t{1} << kReferenceBits;
  constexpr std::size_t kEmptyPopCount = 2 * kExternalModulus + 17;

  std::size_t unexpected_values = 0;
  for (std::size_t i = 0; i < kEmptyPopCount; ++i)
    unexpected_values += queue.pop().has_value() ? 1U : 0U;

  EXPECT_EQ(unexpected_values, 0U);

  const auto after = queue.m_head.load(std::memory_order_relaxed);
  EXPECT_EQ(after.node, drained_head.node);
  EXPECT_EQ(after.thread_ref_counter, 1)
      << "empty pop leaked references into the bounded packed counter";
  EXPECT_EQ(after.node->inner_ref_counter.get_threads_ref(), 0);
  EXPECT_EQ(after.node->inner_ref_counter.get_head_tail_ref(), 2);

  // The queue must remain usable after crossing the old wrap threshold.
  queue.push(42);
  auto value = queue.pop();
  ASSERT_TRUE(value.has_value());
  EXPECT_EQ(**value, 42);
  EXPECT_TRUE(queue.empty());
}

// Regression for the former clear() use-after-free: public clear() must drain
// values while preserving a live final dummy, so the queue remains reusable;
// repeated clear and the destructor must then reclaim that dummy only once.
TEST(LockFreeRefQueueTest, ClearDrainsAndQueueCanBeReused) {
  concurrency::ConcurrentQueue<int> queue;

  for (int value = 0; value < 5; ++value)
    queue.push(value);

  queue.clear();
  EXPECT_TRUE(queue.empty());
  EXPECT_EQ(queue.size(), 0U);

  queue.push(42);
  auto value = queue.pop();
  ASSERT_TRUE(value.has_value());
  EXPECT_EQ(**value, 42);

  // Repeated clear and the eventual destructor must not reclaim the same dummy
  // more than once.
  queue.clear();
  EXPECT_TRUE(queue.empty());
  EXPECT_EQ(queue.size(), 0U);
}
