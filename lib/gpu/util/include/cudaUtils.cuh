#pragma once
#ifndef _CUDA_UTILS_CUH_
#define _CUDA_UTILS_CUH_
#include <cuda.h>
#include <cuda_runtime.h>

namespace gpu {
namespace util {

// input ny rows, nx columns
// output nx rows, ny columns;
// block is BlockSize × BlockSize
template <typename _Ty, std::size_t BlockSize>
__global__ void kernel_transpose(_Ty *out, const _Ty *in, int nx, int ny) {
  int x = blockIdx.x * BlockSize + threadIdx.x;
  int y = blockIdx.y * BlockSize + threadIdx.y;
  int rx = blockIdx.y * BlockSize + threadIdx.x;
  int ry = blockIdx.x * BlockSize + threadIdx.y;
  __shared__ volatile _Ty tile[BlockSize][BlockSize + 1];
  if (x < nx && y < ny)
    tile[threadIdx.y][threadIdx.x] = in[std::size_t(y) * nx + x];

  __syncthreads();

  if (rx < ny && ry < nx)
    out[std::size_t(ry) * ny + rx] = tile[threadIdx.x][threadIdx.y];
}

} // namespace util
} // namespace gpu

#endif //_CUDA_UTILS_CUH_
