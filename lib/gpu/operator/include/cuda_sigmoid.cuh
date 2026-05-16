#pragma once
#ifndef _CUDA_SIGMOID_CUH_
#define _CUDA_SIGMOID_CUH_
#include <cuda_fp16.h>
#include <cuda_runtime.h>

namespace cuda_operator {

namespace details {

__device__ __forceinline__ half2 exp_half2(half2 x);
__device__ __forceinline__ float2 exp_float2(float2 x);

__device__ __forceinline__ float sigmoid_float(float x);
__device__ __forceinline__ float2 sigmoid_float2(float2 x);

__device__ __forceinline__ half sigmoid_half(half x);
__device__ __forceinline__ half sigmoid_half_percise(half x);

__device__ __forceinline__ half2 sigmoid_half2(half2 x);
__device__ __forceinline__ half2 sigmoid_half2_percise(half2 x);
} // namespace details

__global__ void sigmoid_fp16_kernel(half* __restrict src,
          half* __restrict dst, int N);
__global__ void sigmoid_fp16x2_kernel(half* __restrict src,
          half* __restrict dst, int N);
__global__ void sigmoid_fp16x8_unpacked_kernel(half *__restrict src,
                                               half *__restrict dst, int N);
__global__ void sigmoid_fp16x8_packed_kernel(half *__restrict src,
                                             half *__restrict dst, int N);

__global__ void sigmoid_fp32_kernel(float *__restrict src,
          float* __restrict dst, int N);
__global__ void sigmoid_fp32x4_kernel(float* __restrict src,
          float* __restrict dst, int N);
} // namespace cuda_operator

#endif
