#pragma once

#include <cstdint>
#include <thread>

#if defined(__x86_64__) || defined(_M_X64) || defined(__i386__) ||             \
    defined(_M_IX86)
#include <immintrin.h>
#endif

namespace coreforge::queue_benchmark {

class ExponentialBackoff {
public:
  void wait() noexcept {
    if (attempt_ >= kYieldAfterAttempts) {
      std::this_thread::yield();
      return;
    }

    const std::uint32_t shift =
        attempt_ < kMaximumPauseShift ? attempt_ : kMaximumPauseShift;
    const std::uint32_t pause_count = std::uint32_t{1} << shift;
    for (std::uint32_t i = 0; i < pause_count; ++i)
      relax_once();

    ++attempt_;
  }

  void reset() noexcept { attempt_ = 0; }

  // Precise time-based waits should use one processor hint rather than the
  // adaptive wait(), whose eventual OS yield can overshoot a scheduled time.
  static void relax_once() noexcept {
#if defined(__aarch64__) || defined(_M_ARM64)
    asm volatile("yield" ::: "memory");
#elif defined(__x86_64__) || defined(_M_X64) || defined(__i386__) ||           \
    defined(_M_IX86)
    _mm_pause();
#else
    std::this_thread::yield();
#endif
  }

private:
  static constexpr std::uint32_t kMaximumPauseShift = 6;
  static constexpr std::uint32_t kYieldAfterAttempts = 10;

  std::uint32_t attempt_{};
};

} // namespace coreforge::queue_benchmark
