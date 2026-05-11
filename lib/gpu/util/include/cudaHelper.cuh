#pragma once
#ifndef _CUDA_HELPER_HPP_
#define _CUDA_HELPER_HPP_
#include <cuda_fp16.h>
#include <linearUtils.cuh>
#include <stdexcept>
#include <string>
#include <system_error>

namespace cudahelper {
inline const std::error_category &cudaErrorCategory() noexcept {
  struct MyCudaErrorCategory : std::error_category {
    std::string message(int val) const override {
      return cudaGetErrorString(static_cast<cudaError_t>(val));
    }
    const char *name() const noexcept override { return "CUDA"; }
  };
  static MyCudaErrorCategory ins;
  return ins;
}

inline std::error_code makeCudaError(const cudaError_t error) noexcept {
  return {static_cast<int>(error), cudaErrorCategory()};
}
inline void throwCudaError(const cudaError_t e, const char *file, int line) {
  throw std::system_error(makeCudaError(e), std::string(file ? file : "[??]") +
                                                ":" + std::to_string(line));
}

#define GLOBAL_LINEAR_TID util::global_linear_tid()
#define GLOBAL_LINEAR_STRIDE util::global_linear_stride()

#define MAX_EXP_F32 (88.3762626647949f)
#define MIN_EXP_F32 (-88.3762626647949f)
#define MAX_EXP_F16 (__float2half(11.089866488461016f))
#define MIN_EXP_F16 (__float2half(-9.704060527839234f))

#define EXP_HALF_CLIP(x)                                                       \
  do {                                                                         \
    (x) = __hmin(__hmax(MIN_EXP_F16, (x)), MAX_EXP_F16);                       \
  } while (0)

#define EXP_HALF2_CLIP(in)                                                     \
  do {                                                                         \
    (in.x) = __hmin(__hmax(MIN_EXP_F16, (in.x)), MAX_EXP_F16);                 \
    (in.y) = __hmin(__hmax(MIN_EXP_F16, (in.y)), MAX_EXP_F16);                 \
  } while (0)

template <bool Exclusive>
__device__ __forceinline__ uint32_t warp_scan(uint32_t v, int lane) {
  // inclusive scan first
#ifdef __CUDACC__
#pragma unroll
#endif
  for (int off = 1; off < 32; off <<= 1) {
    uint32_t n{};
#ifdef __CUDACC__
    n = __shfl_up_sync(0xffffffffu, v, off, 32);
#endif
    if (lane >= off)
      v += n;
  }

  if constexpr (Exclusive) {
    // convert inclusive -> exclusive
    uint32_t prev{};
#ifdef __CUDACC__
    prev = __shfl_up_sync(0xffffffffu, v, 1, 32);
#endif
    return (lane == 0) ? 0u : prev;
  } else {
    return v; // inclusive
  }
}

__device__ __forceinline__ uint32_t warp_reduce_sum(uint32_t v) {
#ifdef __CUDACC__
#pragma unroll
#endif
  for (int off = 16; off > 0; off >>= 1)
#ifdef __CUDACC__
    v += __shfl_down_sync(0xffffffffu, v, off);
#endif
  return v;
}

__device__ __forceinline__ uint32_t warp_exclusive_scan(uint32_t v, int lane) {
  return warp_scan<true>(v, lane);
}

__device__ __forceinline__ uint32_t warp_inclusive_scan(uint32_t v, int lane) {
  return warp_scan<false>(v, lane);
}

template <typename Func>
__global__ void parallel_for(std::size_t size, Func func) {
  const auto tid = GLOBAL_LINEAR_TID;
  const auto stride = GLOBAL_LINEAR_STRIDE;
  for (auto i = tid; i < size; i += stride) {
    func(i);
  }
}

template <std::size_t N, typename Func>
__global__ void parallel_for(Func func) {
  parallel_for(N, func);
}

__global__ void kernel_clear_u32(uint32_t *__restrict ptr, std::size_t n);

// Convenience function for checking CUDA runtime API results
// can be wrapped around any runtime API call. No-op in release builds.
inline cudaError_t checkCuda(cudaError_t result) {
  if (result != cudaSuccess) {
    cudahelper::throwCudaError(result, __FILE__, __LINE__);
  }
  return result;
}
} // namespace cudahelper

#endif //_CUDA_HELPER_HPP_
