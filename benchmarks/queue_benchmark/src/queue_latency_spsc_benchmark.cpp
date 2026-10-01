#include <benchmark/benchmark.h>

#include "lib/circular_handoff_queue.hpp"
#include "lib/exponential_backoff.hpp"
#include "lib/linked_handoff_queue.hpp"
#include "lib/queue_latency_common.hpp"

#include <atomic>
#include <chrono>
#include <cstdint>
#include <string>
#include <thread>
#include <utility>
#include <vector>

namespace coreforge::queue_benchmark {

template <typename Queue>
void BM_Queue_SPSC_HandoffLatency(benchmark::State &state) {
  static_assert(Clock::is_steady,
                "handoff latency requires a monotonic steady clock");
  const auto total_items = configured_total_items();
  const auto latency_samples = expected_latency_samples(total_items);

  for (auto _ : state) {
    Queue queue;
    std::atomic<std::size_t> ready{0};
    std::atomic<std::uint64_t> consumed_sequence{0};
    std::atomic<bool> start{false};
    std::atomic<bool> valid{true};

    std::vector<std::int64_t> samples;
    samples.reserve(latency_samples);

    std::thread producer([&] {
      ready.fetch_add(1, std::memory_order_release);
      ExponentialBackoff start_backoff;
      while (!start.load(std::memory_order_acquire))
        start_backoff.wait();

      for (std::uint64_t sequence = 0; sequence < total_items; ++sequence) {
        ExponentialBackoff handoff_backoff;
        while (consumed_sequence.load(std::memory_order_acquire) != sequence)
          handoff_backoff.wait();

        // The timestamp is immediately before the enqueue that publishes this
        // element. With one element in flight, this measures a single
        // publication-to-consumption handoff rather than whole-run latency.
        const auto timestamp = should_sample_spsc(sequence) ? now_ns() : 0;
        queue.push(HandoffValue{sequence, timestamp});
      }
    });

    std::thread consumer([&] {
      ready.fetch_add(1, std::memory_order_release);
      ExponentialBackoff start_backoff;
      while (!start.load(std::memory_order_acquire))
        start_backoff.wait();

      for (std::uint64_t expected = 0; expected < total_items; ++expected) {
        const HandoffValue value = queue.pop();

        if (value.sequence != expected)
          valid.store(false, std::memory_order_relaxed);

        if (value.published_ns != 0) {
          const auto received_ns = now_ns();
          if (value.published_ns > received_ns) {
            valid.store(false, std::memory_order_relaxed);
          } else {
            samples.push_back(received_ns - value.published_ns);
          }
        }

        consumed_sequence.store(expected + 1, std::memory_order_release);
      }
    });

    ExponentialBackoff ready_backoff;
    while (ready.load(std::memory_order_acquire) != 2)
      ready_backoff.wait();

    const auto begin = Clock::now();
    start.store(true, std::memory_order_release);

    consumer.join();
    const auto end = Clock::now();
    producer.join();

    state.SetIterationTime(std::chrono::duration<double>(end - begin).count());

    if (!valid.load(std::memory_order_relaxed) ||
        samples.size() != latency_samples) {
      state.SkipWithError("SPSC handoff validation failed");
      continue;
    }

    report_latency(state, std::move(samples));
  }

  state.SetItemsProcessed(
      static_cast<std::int64_t>(state.iterations() * total_items));
  state.SetLabel(
      std::to_string(total_items) +
      " handoffs; one item in flight; 1/100 sampled after 100K warmup; "
      "unpinned");
}

BENCHMARK_TEMPLATE(BM_Queue_SPSC_HandoffLatency, CircularHandoffQueue)
    ->Iterations(1)
    ->UseManualTime();

BENCHMARK_TEMPLATE(BM_Queue_SPSC_HandoffLatency, LinkedHandoffQueue)
    ->Iterations(1)
    ->UseManualTime();

} // namespace coreforge::queue_benchmark
