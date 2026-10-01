#include <benchmark/benchmark.h>

#include "lib/circular_handoff_queue.hpp"
#include "lib/exponential_backoff.hpp"
#include "lib/linked_handoff_queue.hpp"
#include "lib/queue_latency_common.hpp"

#include <array>
#include <atomic>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <iterator>
#include <limits>
#include <string>
#include <thread>
#include <utility>
#include <vector>

namespace coreforge::queue_benchmark {
namespace {

constexpr std::size_t kMpmcProducers = 4;
constexpr std::size_t kMpmcConsumers = 4;
constexpr std::uint64_t kMpmcOfferedLoadPerSecond = 1'000'000;
constexpr std::uint64_t kStopSequence =
    std::numeric_limits<std::uint64_t>::max();

struct alignas(128) ConsumerStats {
  std::vector<std::int64_t> latency_ns;
  std::uint64_t count{};
  std::uint64_t sum{};
  std::uint64_t xor_value{};
  bool valid{true};
};

[[nodiscard]] constexpr bool
should_sample_mpmc(const std::uint64_t sequence,
                   const std::uint64_t producer_begin) {
  constexpr auto warmup_per_producer = kWarmupItems / kMpmcProducers;
  return sequence >= producer_begin + warmup_per_producer &&
         ((sequence - producer_begin - warmup_per_producer) % kSampleStride ==
          0);
}

} // namespace

template <typename Queue>
void BM_Queue_4P4C_OpenLoopHandoffLatency(benchmark::State &state) {
  static_assert(Clock::is_steady,
                "handoff latency requires a monotonic steady clock");
  const auto total_items = configured_total_items(kMpmcProducers);
  const auto latency_samples = expected_latency_samples(total_items);

  for (auto _ : state) {
    Queue queue;
    std::atomic<std::size_t> ready{0};
    std::atomic<std::size_t> producers_finished{0};
    std::atomic<std::size_t> consumers_finished{0};
    std::atomic<bool> start{false};
    std::int64_t schedule_start_ns{};

    std::array<std::thread, kMpmcProducers> producers;
    std::array<std::thread, kMpmcConsumers> consumers;
    std::array<ConsumerStats, kMpmcConsumers> stats;

    for (auto &consumer_stats : stats) {
      consumer_stats.latency_ns.reserve(latency_samples / kMpmcConsumers +
                                        1024);
    }

    for (std::size_t producer_id = 0; producer_id < kMpmcProducers;
         ++producer_id) {
      producers[producer_id] = std::thread([&, producer_id] {
        ready.fetch_add(1, std::memory_order_release);
        ExponentialBackoff start_backoff;
        while (!start.load(std::memory_order_acquire))
          start_backoff.wait();

        const std::uint64_t begin = total_items * producer_id / kMpmcProducers;
        const std::uint64_t end =
            total_items * (producer_id + 1) / kMpmcProducers;
        const std::int64_t global_interval_ns =
            1'000'000'000LL /
            static_cast<std::int64_t>(kMpmcOfferedLoadPerSecond);
        const std::int64_t producer_interval_ns =
            global_interval_ns * static_cast<std::int64_t>(kMpmcProducers);

        for (std::uint64_t sequence = begin; sequence < end; ++sequence) {
          const auto local_index = sequence - begin;
          const auto scheduled_ns =
              schedule_start_ns +
              static_cast<std::int64_t>(producer_id) * global_interval_ns +
              static_cast<std::int64_t>(local_index) * producer_interval_ns;

          // A scheduled open-loop arrival needs a precise spin hint. Adaptive
          // backoff could yield the OS thread and overshoot the target time.
          while (now_ns() < scheduled_ns)
            ExponentialBackoff::relax_once();

          // Start at the scheduled arrival, not at the producer's actual wake
          // time, so scheduler delay and queueing remain visible in the tail.
          const std::int64_t timestamp =
              should_sample_mpmc(sequence, begin) ? scheduled_ns : 0;
          queue.push(HandoffValue{sequence, timestamp});
        }

        producers_finished.fetch_add(1, std::memory_order_release);
      });
    }

    for (std::size_t consumer_id = 0; consumer_id < kMpmcConsumers;
         ++consumer_id) {
      consumers[consumer_id] = std::thread([&, consumer_id] {
        auto &local = stats[consumer_id];

        ready.fetch_add(1, std::memory_order_release);
        ExponentialBackoff start_backoff;
        while (!start.load(std::memory_order_acquire))
          start_backoff.wait();

        for (;;) {
          const HandoffValue value = queue.pop();
          if (value.sequence == kStopSequence)
            break;

          ++local.count;
          local.sum += value.sequence;
          local.xor_value ^= value.sequence;

          if (value.published_ns != 0) {
            const auto received_ns = now_ns();
            if (value.published_ns > received_ns) {
              local.valid = false;
              local.latency_ns.push_back(0);
            } else {
              local.latency_ns.push_back(received_ns - value.published_ns);
            }
          }
        }

        consumers_finished.fetch_add(1, std::memory_order_release);
      });
    }

    ExponentialBackoff ready_backoff;
    while (ready.load(std::memory_order_acquire) !=
           kMpmcProducers + kMpmcConsumers) {
      ready_backoff.wait();
    }

    schedule_start_ns = now_ns() + 1'000'000;
    const auto begin = Clock::now();
    start.store(true, std::memory_order_release);

    ExponentialBackoff producer_backoff;
    while (producers_finished.load(std::memory_order_acquire) !=
           kMpmcProducers) {
      producer_backoff.wait();
    }

    // All data pushes precede the stop markers. FIFO order therefore drains
    // the data stream before each consumer exits on one marker.
    for (std::size_t i = 0; i < kMpmcConsumers; ++i)
      queue.push(HandoffValue{kStopSequence, 0});

    ExponentialBackoff consumer_backoff;
    while (consumers_finished.load(std::memory_order_acquire) !=
           kMpmcConsumers) {
      consumer_backoff.wait();
    }

    const auto end = Clock::now();

    for (auto &producer : producers)
      producer.join();
    for (auto &consumer : consumers)
      consumer.join();

    state.SetIterationTime(std::chrono::duration<double>(end - begin).count());

    std::uint64_t count = 0;
    std::uint64_t sum = 0;
    std::uint64_t xor_value = 0;
    std::vector<std::int64_t> samples;
    samples.reserve(latency_samples);

    for (auto &local : stats) {
      if (!local.valid)
        state.SkipWithError("4P4C observed a non-monotonic timestamp");
      count += local.count;
      sum += local.sum;
      xor_value ^= local.xor_value;
      samples.insert(samples.end(),
                     std::make_move_iterator(local.latency_ns.begin()),
                     std::make_move_iterator(local.latency_ns.end()));
    }

    const auto expected_sum = total_items * (total_items - 1) / 2;
    const auto expected_xor = xor_zero_to(total_items - 1);

    if (state.skipped())
      continue;

    if (count != total_items || sum != expected_sum ||
        xor_value != expected_xor || samples.size() != latency_samples) {
      state.SkipWithError("4P4C count/checksum/xor/latency validation failed");
      continue;
    }

    report_latency(state, std::move(samples));
  }

  state.SetItemsProcessed(
      static_cast<std::int64_t>(state.iterations() * total_items));
  state.counters["offered_load_items_per_second"] =
      static_cast<double>(kMpmcOfferedLoadPerSecond);
  state.SetLabel(std::to_string(total_items) +
                 " handoffs; 4P4C open-loop at 1M/s; scheduled-arrival to pop; "
                 "1/100 sampled after 25K/producer warmup; unpinned");
}

BENCHMARK_TEMPLATE(BM_Queue_4P4C_OpenLoopHandoffLatency, CircularHandoffQueue)
    ->Iterations(1)
    ->UseManualTime();

BENCHMARK_TEMPLATE(BM_Queue_4P4C_OpenLoopHandoffLatency, LinkedHandoffQueue)
    ->Iterations(1)
    ->UseManualTime();

} // namespace coreforge::queue_benchmark
