#include <SparseDS.hpp>

#include <atomic>
#include <cassert>
#include <cstdint>
#include <thread>
#include <vector>

namespace {

using HashDenseGrid =
    sparse::RootGrid<int, sparse::HashBlock<sparse::DenseBlock<16, int>>>;

void testHashDenseReadWrite() {
  HashDenseGrid grid;

  grid.write(3, 7, 11);
  grid.write(-17, 35, 29);

  assert(grid.read(3, 7) == 11);
  assert(grid.read(-17, 35) == 29);
  assert(!grid.read(1024, 1024).has_value());
}

void testDenseBoolReferences() {
  using BoolGrid =
      sparse::RootGrid<bool, sparse::HashBlock<sparse::DenseBlock<16, bool>>>;

  BoolGrid grid;
  grid.write(-1, -1, true);
  grid.write(31, 47, true);

  assert(grid.read(-1, -1).value_or(false));
  assert(grid.read(31, 47).value_or(false));
}

void testPointerDenseReadWrite() {
  using PointerGrid =
      sparse::RootGrid<int,
                       sparse::PointerBlock<8, sparse::DenseBlock<4, int>>>;

  PointerGrid grid;
  grid.write(3, 7, 13);
  grid.write(20, 17, 41);
  grid.write(-1, -1, 73);

  assert(grid.read(3, 7) == 13);
  assert(grid.read(20, 17) == 41);
  assert(grid.read(-1, -1) == 73);
}

void testConcurrentWritesAndTraversal() {
  HashDenseGrid grid;
  constexpr std::intptr_t threadCount = 8;
  constexpr std::intptr_t valuesPerThread = 128;

  std::vector<std::thread> workers;
  workers.reserve(threadCount);
  for (std::intptr_t thread = 0; thread < threadCount; ++thread) {
    workers.emplace_back([&, thread] {
      for (std::intptr_t index = 0; index < valuesPerThread; ++index) {
        const auto x = thread * 4096 + index;
        grid.write(x, thread, static_cast<int>(x + thread + 1));
      }
    });
  }
  for (auto &worker : workers) {
    worker.join();
  }

  for (std::intptr_t thread = 0; thread < threadCount; ++thread) {
    for (std::intptr_t index = 0; index < valuesPerThread; ++index) {
      const auto x = thread * 4096 + index;
      assert(grid.read(x, thread) == static_cast<int>(x + thread + 1));
    }
  }

  std::atomic<std::size_t> nonZeroValues{0};
  grid.foreach ([&](auto, auto, const auto &value) {
    if (value != 0) {
      nonZeroValues.fetch_add(1, std::memory_order_relaxed);
    }
  });
  assert(nonZeroValues == threadCount * valuesPerThread);
}

} // namespace

int main() {
  testHashDenseReadWrite();
  testDenseBoolReferences();
  testPointerDenseReadWrite();
  testConcurrentWritesAndTraversal();
}
