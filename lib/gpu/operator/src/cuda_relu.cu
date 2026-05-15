#include <cuda_relu.cuh>
#include <float.h>

namespace cuda_operator {

namespace details {
__device__ __forceinline__ float relu_float(float x) { return fmaxf(0.f, x); }
} // namespace details

__global__ void relu_fp32_kernel(const float *__restrict__ src,
                                 float *__restrict__ dst, int N) {
  int idx = threadIdx.x + blockIdx.x * blockDim.x;
  if (idx < N)
    dst[idx] = details::relu_float(src[idx]);
}
} // namespace cuda_operator
