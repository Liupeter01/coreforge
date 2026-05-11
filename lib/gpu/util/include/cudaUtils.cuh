#pragma once
#ifndef _CUDA_UTILS_CUH_
#define _CUDA_UTILS_CUH_
#include <cuda.h>
#include <cuda_runtime.h>

namespace gpu {
namespace util {
template <typename _Ty, std::size_t BlockSize>
__global__ void kernel_transpose(_Ty *__restrict out, const _Ty *__restrict in,
                                 const uint32_t width, const uint32_t height) {

  // before transpose
  const uint32_t x = blockIdx.x * BlockSize + threadIdx.x;
  const uint32_t y = blockIdx.y * BlockSize + threadIdx.y;

  __shared__ volatile _Ty tile[BlockSize][BlockSize + 1];

  _Ty value = _Ty(0);
  if (x < width && y < height) {
    value = in[y * width + x];
  }

  // after transpose, we move calculation here to cover memory latency
  const uint32_t tx = blockIdx.y * BlockSize + threadIdx.x;
  const uint32_t ty = blockIdx.x * BlockSize + threadIdx.y;

  tile[threadIdx.y][threadIdx.x] = value;

  __syncthreads();

  if (tx < height && ty < width) {
    out[ty * height + tx] = tile[threadIdx.x][threadIdx.y];
  }
}
} // namespace util
} // namespace gpu

#endif //_CUDA_UTILS_CUH_
