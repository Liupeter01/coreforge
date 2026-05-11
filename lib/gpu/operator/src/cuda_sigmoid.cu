#include <cuda_sigmoid.cuh>
#include <cmath>
#include <cudaHelper.cuh>

namespace cuda_operator {

          namespace details {

                    static constexpr float ONE = 1.f;

                    __device__ __forceinline__ half2 exp_half2(half2 x) {
                              return make_half2(hexp(x.x), hexp(x.y));
                    }

                    __device__ __forceinline__ float2 exp_float2(float2 x){
                              return make_float2(expf(x.x), expf(x.y));
                    }

                    __device__ __forceinline__ float sigmoid_float(float x) {
                              return ONE / (ONE + expf(-x));
                    }

                    __device__ __forceinline__ float2 sigmoid_float2(float2 x){
                              float res1 = ONE / (ONE + expf(-x.x));
                              float res2 = ONE / (ONE + expf(-x.y));
                              return make_float2(res1, res2);
                    }

                    __device__ __forceinline__ half sigmoid_half(half x) {
                              const half one = __float2half_rn(ONE);
                              return __hdiv(one, one + hexp(-x));
                    }

                    __device__ __forceinline__ half sigmoid_half_percise(half x) {
                              return __float2half_rn(sigmoid_half(__half2float(x)));
                    }

                    __device__ __forceinline__ half2 sigmoid_half2(half2 x) {

                              const half one = __float2half_rn(ONE);
                              half2 one2 = make_half2(one, one);

                              return __h2div(one2, one2 + h2exp(-x));
                    }

                    __device__ __forceinline__ half2 sigmoid_half2_percise(half2 x){
                              return __float22half2_rn(sigmoid_float2(__half22float2(x)));
                    }
          }

          __global__ void sigmoid_fp16x8_unpacked_kernel(half* __restrict src, half* __restrict dst, int N) {
                    const int idx = 8 * (threadIdx.x + blockDim.x * blockIdx.x);

                    half2 tmp[4]{}, result[4]{};

                    if ((idx + 0) < N)  tmp[0] = *(reinterpret_cast<half2*>(&src[idx + 0]));
                    if ((idx + 2) < N)  tmp[1] = *(reinterpret_cast<half2*>(&src[idx + 2]));
                    if ((idx + 4) < N)  tmp[2] = *(reinterpret_cast<half2*>(&src[idx + 4]));
                    if ((idx + 6) < N)  tmp[3] = *(reinterpret_cast<half2*>(&src[idx + 6]));

#pragma unroll 4
                    for (int i = 0; i < 4; ++i) {
                              EXP_HALF2_CLIP(tmp[i]);
                              result[i] = details::sigmoid_half2(tmp[i]);
                    }

                    if ((idx + 0) < N) *(reinterpret_cast<half2*>(&dst[idx + 0])) = result[0];
                    if ((idx + 2) < N) *(reinterpret_cast<half2*>(&dst[idx + 2])) = result[1];
                    if ((idx + 4) < N) *(reinterpret_cast<half2*>(&dst[idx + 4])) = result[2];
                    if ((idx + 6) < N) *(reinterpret_cast<half2*>(&dst[idx + 6])) = result[3];
          }

          __global__ void sigmoid_fp16x8_packed_kernel(half* __restrict src, half* __restrict dst, int N) {
                    const int idx = 8 * (threadIdx.x + blockDim.x * blockIdx.x);

                    //safety consideration
                    if (idx + 7 >= N) {
                              return;
                    }

                    half2 tmp[4]{};
                    float4 out = make_float4(0.f, 0.f, 0.f, 0.f);

                    *reinterpret_cast<float4*>(&tmp[0]) = *reinterpret_cast<float4*>(&src[idx]);

#pragma unroll 4
                    for (int i = 0; i < 4; ++i) {
                              EXP_HALF2_CLIP(tmp[i]);
                              *(reinterpret_cast<half2*>(&out) + i) = details::sigmoid_half2(tmp[i]);
                    }

                    *(reinterpret_cast<float4*>(&dst[idx + 0])) = out;
          }
} 