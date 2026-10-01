#include <benchmark/benchmark.h>
#include <circular_queue_lockfree.hpp>
#include <queue_lk.hpp>
#include <queue_lockfree.hpp>
#include <thread>

static void BM_CircularQueue_PushPopSingleThread(benchmark::State &state) {
  concurrency::ConcurrentCircularQueue<std::uint64_t, 65536> queue;
  std::uint64_t output{};

  for (auto _ : state) {
    while (!queue.push(42))
      ;

    const bool popped = queue.pop(output);
    benchmark::DoNotOptimize(popped);
    benchmark::DoNotOptimize(output);
  }
  state.SetItemsProcessed(state.iterations());
}

static void BM_LinklistQueue_PushPopSingleThread(benchmark::State &state) {
  concurrency::ConcurrentQueue<std::uint64_t> queue;

  for (auto _ : state) {

    queue.push(42);

    auto it = queue.pop();
    if (it.has_value()) {
      benchmark::DoNotOptimize(**it);
    }
    benchmark::DoNotOptimize(it);
  }
  state.SetItemsProcessed(state.iterations());
}

static void BM_MutexQueue_PushPopSingleThread(benchmark::State &state) {
  concurrency::ConcurrentQueueLk<std::uint64_t> queue;

  for (auto _ : state) {

    queue.push(42);

    auto it = queue.pop();
    benchmark::DoNotOptimize(*it);
  }
  state.SetItemsProcessed(state.iterations());
}

template <std::size_t Producers, std::size_t Consumers,
          std::size_t N = 10'000'000>
static void BM_CircularQueue_MPMC(benchmark::State &state) {

  for (auto _ : state) {
    concurrency::ConcurrentCircularQueue<std::uint64_t, 65536> queue;

    std::atomic<std::size_t> ready{0};
    std::atomic<std::size_t> finished{0};
    std::atomic<std::size_t> producers_done{0};
    std::atomic<bool> start{false};

    std::vector<std::thread> threads;

    struct alignas(64) ConsumCacheline {
      std::size_t value{};
    };

    struct alignas(64) ChecksumCacheline {
      std::uint64_t value{};
    };

    std::array<ConsumCacheline, Consumers> consumed{};
    std::array<ChecksumCacheline, Consumers> checksums{};

    // producer
    for (std::size_t p = 0; p < Producers; ++p) {
      threads.emplace_back([&, p] {
        ready.fetch_add(1, std::memory_order_release);
        while (!start.load(std::memory_order_acquire))
          std::this_thread::yield();

        const std::size_t begin = N * p / Producers;
        const std::size_t end = N * (p + 1) / Producers;

        for (std::size_t value = begin; value < end; ++value) {
          while (!queue.push(value))
            ;
        }

        producers_done.fetch_add(1, std::memory_order_release);
        finished.fetch_add(1, std::memory_order_release);
      });
    }

    // consumer
    for (std::size_t c = 0; c < Consumers; ++c) {
      threads.emplace_back([&, c] {
        std::size_t local_consumed = 0;
        std::uint64_t local_checksum = 0;
        std::uint64_t value{};

        ready.fetch_add(1, std::memory_order_release);
        while (!start.load(std::memory_order_acquire))
          std::this_thread::yield();

        for (;;) {
          if (queue.pop(value)) {
            ++local_consumed;
            local_checksum += value;
            continue;
          }

          if (producers_done.load(std::memory_order_acquire) == Producers)
            break;
        }

        consumed[c].value = local_consumed;
        checksums[c].value = local_checksum;
        finished.fetch_add(1, std::memory_order_release);
      });
    }

    while (ready.load(std::memory_order_acquire) != Producers + Consumers)
      std::this_thread::yield();

    const auto begin = std::chrono::steady_clock::now();

    // start the benchmark from here!!
    start.store(true, std::memory_order_release);

    while (finished.load(std::memory_order_acquire) != Producers + Consumers)
      std::this_thread::yield();

    const auto end = std::chrono::steady_clock::now();

    // stop the timer after the benchmark is done, so that thread
    // creation/recycling does not enter throughput.
    for (auto &thread : threads)
      thread.join();

    const double seconds = std::chrono::duration<double>(end - begin).count();
    state.SetIterationTime(seconds);

    std::size_t total_consumed = 0;
    std::uint64_t checksum = 0;
    for (std::size_t c = 0; c < Consumers; ++c) {
      total_consumed += consumed[c].value;
      checksum += checksums[c].value;
    }

    benchmark::DoNotOptimize(checksum);

    if (total_consumed != N)
      state.SkipWithError("consumer count mismatch");
  }

  state.SetItemsProcessed(static_cast<std::int64_t>(state.iterations() * N));
}

template <std::size_t Producers, std::size_t Consumers,
          std::size_t N = 10'000'000>
static void BM_LinkListQueue_MPMC(benchmark::State &state) {

  for (auto _ : state) {
    concurrency::ConcurrentQueue<std::uint64_t> queue;

    std::atomic<std::size_t> ready{0};
    std::atomic<std::size_t> finished{0};
    std::atomic<std::size_t> producers_done{0};
    std::atomic<bool> start{false};

    std::vector<std::thread> threads;

    struct alignas(64) ConsumCacheline {
      std::size_t value{};
    };

    struct alignas(64) ChecksumCacheline {
      std::uint64_t value{};
    };

    std::array<ConsumCacheline, Consumers> consumed{};
    std::array<ChecksumCacheline, Consumers> checksums{};

    // producer
    for (std::size_t p = 0; p < Producers; ++p) {
      threads.emplace_back([&, p] {
        ready.fetch_add(1, std::memory_order_release);
        while (!start.load(std::memory_order_acquire))
          std::this_thread::yield();

        const std::size_t begin = N * p / Producers;
        const std::size_t end = N * (p + 1) / Producers;

        for (std::size_t value = begin; value < end; ++value) {
          queue.push(value);
        }

        producers_done.fetch_add(1, std::memory_order_release);
        finished.fetch_add(1, std::memory_order_release);
      });
    }

    // consumer
    for (std::size_t c = 0; c < Consumers; ++c) {
      threads.emplace_back([&, c] {
        std::size_t local_consumed = 0;
        std::uint64_t local_checksum = 0;
        std::uint64_t value{};

        ready.fetch_add(1, std::memory_order_release);
        while (!start.load(std::memory_order_acquire))
          std::this_thread::yield();

        for (;;) {

          auto it = queue.pop();
          if (it.has_value()) {
            ++local_consumed;
            local_checksum += (**it);
            continue;
          }

          if (producers_done.load(std::memory_order_acquire) == Producers)
            break;
        }

        consumed[c].value = local_consumed;
        checksums[c].value = local_checksum;
        finished.fetch_add(1, std::memory_order_release);
      });
    }

    while (ready.load(std::memory_order_acquire) != Producers + Consumers)
      std::this_thread::yield();

    const auto begin = std::chrono::steady_clock::now();

    // start the benchmark from here!!
    start.store(true, std::memory_order_release);

    while (finished.load(std::memory_order_acquire) != Producers + Consumers)
      std::this_thread::yield();

    const auto end = std::chrono::steady_clock::now();

    // stop the timer after the benchmark is done, so that thread
    // creation/recycling does not enter throughput.
    for (auto &thread : threads)
      thread.join();

    const double seconds = std::chrono::duration<double>(end - begin).count();
    state.SetIterationTime(seconds);

    std::size_t total_consumed = 0;
    std::uint64_t checksum = 0;
    for (std::size_t c = 0; c < Consumers; ++c) {
      total_consumed += consumed[c].value;
      checksum += checksums[c].value;
    }

    benchmark::DoNotOptimize(checksum);

    if (total_consumed != N)
      state.SkipWithError("consumer count mismatch");
  }

  state.SetItemsProcessed(static_cast<std::int64_t>(state.iterations() * N));
}

template <std::size_t Producers, std::size_t Consumers,
          std::size_t N = 10'000'000>
static void BM_MutexQueue_MPMC(benchmark::State &state) {

  for (auto _ : state) {
    concurrency::ConcurrentQueueLk<std::uint64_t> queue;

    std::atomic<std::size_t> ready{0};
    std::atomic<std::size_t> finished{0};
    std::atomic<std::size_t> producers_done{0};
    std::atomic<bool> start{false};

    std::vector<std::thread> threads;

    struct alignas(64) ConsumCacheline {
      std::size_t value{};
    };

    struct alignas(64) ChecksumCacheline {
      std::uint64_t value{};
    };

    std::array<ConsumCacheline, Consumers> consumed{};
    std::array<ChecksumCacheline, Consumers> checksums{};

    // producer
    for (std::size_t p = 0; p < Producers; ++p) {
      threads.emplace_back([&, p] {
        ready.fetch_add(1, std::memory_order_release);
        while (!start.load(std::memory_order_acquire))
          std::this_thread::yield();

        const std::size_t begin = N * p / Producers;
        const std::size_t end = N * (p + 1) / Producers;

        for (std::size_t value = begin; value < end; ++value) {
          queue.push(value);
        }

        producers_done.fetch_add(1, std::memory_order_release);
        finished.fetch_add(1, std::memory_order_release);
      });
    }

    // consumer
    for (std::size_t c = 0; c < Consumers; ++c) {
      threads.emplace_back([&, c] {
        std::size_t local_consumed = 0;
        std::uint64_t local_checksum = 0;
        std::uint64_t value{};

        ready.fetch_add(1, std::memory_order_release);
        while (!start.load(std::memory_order_acquire))
          std::this_thread::yield();

        for (;;) {

          auto it = queue.pop();
          if (it) {
            ++local_consumed;
            local_checksum += (*it);
            continue;
          }

          if (producers_done.load(std::memory_order_acquire) == Producers)
            break;
        }

        consumed[c].value = local_consumed;
        checksums[c].value = local_checksum;
        finished.fetch_add(1, std::memory_order_release);
      });
    }

    while (ready.load(std::memory_order_acquire) != Producers + Consumers)
      std::this_thread::yield();

    const auto begin = std::chrono::steady_clock::now();

    // start the benchmark from here!!
    start.store(true, std::memory_order_release);

    while (finished.load(std::memory_order_acquire) != Producers + Consumers)
      std::this_thread::yield();

    const auto end = std::chrono::steady_clock::now();

    // stop the timer after the benchmark is done, so that thread
    // creation/recycling does not enter throughput.
    for (auto &thread : threads)
      thread.join();

    const double seconds = std::chrono::duration<double>(end - begin).count();
    state.SetIterationTime(seconds);

    std::size_t total_consumed = 0;
    std::uint64_t checksum = 0;
    for (std::size_t c = 0; c < Consumers; ++c) {
      total_consumed += consumed[c].value;
      checksum += checksums[c].value;
    }

    benchmark::DoNotOptimize(checksum);

    if (total_consumed != N)
      state.SkipWithError("consumer count mismatch");
  }

  state.SetItemsProcessed(static_cast<std::int64_t>(state.iterations() * N));
}

BENCHMARK(BM_CircularQueue_PushPopSingleThread)
    ->MinTime(3.0)
    ->Repetitions(10)
    ->ReportAggregatesOnly();

BENCHMARK_TEMPLATE(BM_CircularQueue_MPMC, 1, 1, 100'000'000)
    ->Iterations(1)
    ->Repetitions(10)
    ->UseManualTime();

BENCHMARK_TEMPLATE(BM_CircularQueue_MPMC, 2, 2, 100'000'000)
    ->Iterations(1)
    ->Repetitions(10)
    ->UseManualTime();

BENCHMARK_TEMPLATE(BM_CircularQueue_MPMC, 4, 4, 100'000'000)
    ->Iterations(1)
    ->Repetitions(10)
    ->UseManualTime();

BENCHMARK(BM_LinklistQueue_PushPopSingleThread)
    ->MinTime(3.0)
    ->Repetitions(10)
    ->ReportAggregatesOnly();

BENCHMARK_TEMPLATE(BM_LinkListQueue_MPMC, 1, 1, 100'000'000)
    ->Iterations(1)
    ->Repetitions(10)
    ->UseManualTime();

BENCHMARK_TEMPLATE(BM_LinkListQueue_MPMC, 2, 2, 100'000'000)
    ->Iterations(1)
    ->Repetitions(10)
    ->UseManualTime();

BENCHMARK_TEMPLATE(BM_LinkListQueue_MPMC, 4, 4, 100'000'000)
    ->Iterations(1)
    ->Repetitions(10)
    ->UseManualTime();

BENCHMARK(BM_MutexQueue_PushPopSingleThread)
    ->MinTime(3.0)
    ->Repetitions(10)
    ->ReportAggregatesOnly();

BENCHMARK_TEMPLATE(BM_MutexQueue_MPMC, 1, 1, 100'000'000)
    ->Iterations(1)
    ->Repetitions(10)
    ->UseManualTime();

BENCHMARK_TEMPLATE(BM_MutexQueue_MPMC, 2, 2, 100'000'000)
    ->Iterations(1)
    ->Repetitions(10)
    ->UseManualTime();

BENCHMARK_TEMPLATE(BM_MutexQueue_MPMC, 4, 4, 100'000'000)
    ->Iterations(1)
    ->Repetitions(10)
    ->UseManualTime();

BENCHMARK_MAIN();
