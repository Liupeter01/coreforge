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
}

#endif