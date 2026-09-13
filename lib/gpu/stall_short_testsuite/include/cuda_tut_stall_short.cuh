#pragma once
#ifndef _CUDA_TUT_STALL_SHORT_CUH_
#define _CUDA_TUT_STALL_SHORT_CUH_
#include <cuda_runtime.h>

// Only For Reference
// template <typename _Ty>
//__global__ void parallel_transpose(_Ty *out, const _Ty *in, int nx, int ny) {
//   int linear = blockIdx.x * blockDim.x + threadIdx.x;
//   int x = linear % nx;
//   int y = linear / nx;
//   if (x >= nx || y >= ny)
//     return;
//   out[nx * y + x] = in[nx * x + y];
// }
//
// template <typename _Ty>
//__global__ void parallel_transpose_dim3(_Ty *out, const _Ty *in, int nx,
//                                         int ny) {
//   int x = blockIdx.x * blockDim.x + threadIdx.x;
//   int y = blockIdx.y * blockDim.y + threadIdx.y;
//   if (x >= nx || y >= ny)
//     return;
//   out[nx * y + x] = in[nx * x + y];
// }

// input ny rows nx columns
// output nx rows ny columns
// block = BlockSize × BlockSize
template <typename _Ty, std::size_t BlockSize>
__global__ void parallel_transpose_shared(_Ty *out, const _Ty *in, int nx,
                                          int ny) {
  int x = blockIdx.x * BlockSize + threadIdx.x;
  int y = blockIdx.y * BlockSize + threadIdx.y;
  int rx = blockIdx.y * BlockSize + threadIdx.x;
  int ry = blockIdx.x * BlockSize + threadIdx.y;
  __shared__ volatile _Ty buffer[BlockSize * BlockSize];

  if (x < nx && y < ny)
    buffer[threadIdx.y * BlockSize + threadIdx.x] = in[std::size_t(y) * nx + x];

  __syncthreads();
  if (rx < ny && ry < nx)
    out[std::size_t(ry) * ny + rx] =
        buffer[threadIdx.x * BlockSize + threadIdx.y];
}

// input ny rows nx columns
// output nx rows ny columns
// block = BlockSize × BlockSize
template <typename _Ty, std::size_t BlockSize>
__global__ void parallel_transpose_solv_conflict(_Ty *out, const _Ty *in,
                                                 int nx, int ny) {
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

#endif //_CUDA_TUT_STALL_SHORT_CUH_
