#pragma once
#ifndef _CUDA_RELU_CUH_
#define _CUDA_RELU_CUH_
#include <cuda_fp16.h>
#include <cuda_runtime.h>

namespace cuda_operator {

          namespace details {
                    __device__ __forceinline__ float relu_float(float x);
          }

          __global__ void relu_fp32_kernel(const float* __restrict__ src, float* __restrict__ dst,  int N );

          template <typename scalar_t>
          __global__ void relu_generic_kernel(
                    const scalar_t* __restrict__ src,
                    scalar_t* __restrict__ dst,
                    int N ) {
                    int idx = threadIdx.x + blockIdx.x * blockDim.x;
                    if (idx < N) {
                              dst[idx] = src[idx] > scalar_t{ 0 } ? src[idx] : scalar_t{ 0 };
                    }
          }
}

#endif