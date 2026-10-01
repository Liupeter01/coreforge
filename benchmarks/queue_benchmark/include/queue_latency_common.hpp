#pragma once

#include <benchmark/benchmark.h>

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <string>
#include <utility>
#include <vector>

namespace coreforge::queue_benchmark {

using Clock = std::chrono::steady_clock;

inline constexpr std::size_t kDefaultTotalItems = 100'000'000;
inline constexpr std::size_t kWarmupItems = 100'000;
inline constexpr std::size_t kSampleStride = 100;

struct HandoffValue {
  std::uint64_t sequence{};
  std::int64_t published_ns{};
};

[[nodiscard]] inline std::int64_t now_ns() noexcept {
  return std::chrono::duration_cast<std::chrono::nanoseconds>(
             Clock::now().time_since_epoch())
      .count();
}

[[nodiscard]] inline std::uint64_t
configured_total_items(const std::uint64_t required_multiple = 1) noexcept {
  const char *text = std::getenv("COREFORGE_QUEUE_LATENCY_ITEMS");
  if (!text)
    return static_cast<std::uint64_t>(kDefaultTotalItems);

  char *end = nullptr;
  const auto parsed = std::strtoull(text, &end, 10);
  if (end == text || *end != '\0' || parsed <= kWarmupItems ||
      parsed % required_multiple != 0) {
    return static_cast<std::uint64_t>(kDefaultTotalItems);
  }
  return static_cast<std::uint64_t>(parsed);
}

[[nodiscard]] constexpr bool should_sample_spsc(const std::uint64_t sequence) {
  return sequence >= kWarmupItems &&
         ((sequence - kWarmupItems) % kSampleStride == 0);
}

[[nodiscard]] constexpr std::size_t
expected_latency_samples(const std::uint64_t total_items) {
  return static_cast<std::size_t>(
      (total_items - kWarmupItems + kSampleStride - 1) / kSampleStride);
}

[[nodiscard]] constexpr std::uint64_t xor_zero_to(const std::uint64_t n) {
  switch (n & 3U) {
  case 0:
    return n;
  case 1:
    return 1;
  case 2:
    return n + 1;
  default:
    return 0;
  }
}

[[nodiscard]] inline double
nearest_rank_percentile(const std::vector<std::int64_t> &sorted_samples,
                        const double percentile) {
  const auto rank = static_cast<std::size_t>(
      std::ceil(percentile * static_cast<double>(sorted_samples.size())));
  const auto index =
      std::min(sorted_samples.size() - 1, std::max<std::size_t>(rank, 1) - 1);
  return static_cast<double>(sorted_samples[index]);
}

inline void report_latency(benchmark::State &state,
                           std::vector<std::int64_t> samples) {
  std::sort(samples.begin(), samples.end());

  state.counters["handoff_p50_ns"] = nearest_rank_percentile(samples, 0.50);
  state.counters["handoff_p90_ns"] = nearest_rank_percentile(samples, 0.90);
  state.counters["handoff_p99_ns"] = nearest_rank_percentile(samples, 0.99);
  state.counters["handoff_p999_ns"] = nearest_rank_percentile(samples, 0.999);
  state.counters["handoff_max_ns"] = static_cast<double>(samples.back());
  state.counters["latency_samples"] = static_cast<double>(samples.size());
}

} // namespace coreforge::queue_benchmark
