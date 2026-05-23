#include "cuda_relu.cuh"

#include <cuda.h>
#include <cuda_runtime.h>

#if defined(USE_TORCH)
#include <c10/cuda/CUDAGuard.h>
#include <torch/extension.h>
#include <torch/types.h>
#endif

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

template <typename scalar_t>
__global__ void relu_generic_kernel(const scalar_t *__restrict__ src,
                                    scalar_t *__restrict__ dst, int N) {
  int idx = threadIdx.x + blockIdx.x * blockDim.x;
  if (idx >= N)
    return;

  scalar_t x = src[idx];

  if constexpr (std::is_same_v<scalar_t, __nv_bfloat16>
#ifdef USE_TORCH
                || std::is_same_v<scalar_t, c10::BFloat16>
#endif
  ) {
    const auto zero = __float2bfloat16(0.0f);
    dst[idx] = __hgt(x, zero) ? x : zero;
  } else if constexpr (std::is_same_v<scalar_t, __half>
#ifdef USE_TORCH
                       || std::is_same_v<scalar_t, c10::Half>
#endif
  ) {
    const auto zero = __float2half(0.0f);
    dst[idx] = __hgt(x, zero) ? x : zero;
  } else {
    dst[idx] = (x > static_cast<scalar_t>(0)) ? x : static_cast<scalar_t>(0);
  }
}

} // namespace cuda_operator

#if defined(USE_TORCH)
torch::Tensor relu_f32(const torch::Tensor &src) {

  TORCH_CHECK(src.dtype() == torch::kFloat32,
              "relu_f32: expected Float32 tensor, got ", src.dtype());
  TORCH_CHECK(src.is_cuda(), "relu_f32: expected CUDA tensor, got CPU tensor");

  auto src_contig = src.contiguous();

  // make sure kernel executed on the correct device!
  const at::cuda::CUDAGuard device_guard(src_contig.device());

  // create dst tensor which is same as src_contig
  torch::Tensor dst = torch::empty_like(src_contig);

  auto total = src_contig.numel();
  if (!total)
    return dst;

  constexpr int BLOCK_SIZE = 256;
  int grid = (total + BLOCK_SIZE - 1) / BLOCK_SIZE;
  int block = BLOCK_SIZE;

  cuda_operator::relu_fp32_kernel<<<grid, block>>>(src_contig.data_ptr<float>(),
                                                   dst.data_ptr<float>(),
                                                   static_cast<int>(total));

  C10_CUDA_KERNEL_LAUNCH_CHECK();

  return dst;
}

torch::Tensor relu_generic(const torch::Tensor &src) {
  TORCH_CHECK(src.is_cuda(), "relu_f32: expected CUDA tensor, got CPU tensor");

  auto src_contig = src.contiguous();

  // make sure kernel executed on the correct device!
  const at::cuda::CUDAGuard device_guard(src_contig.device());

  // create dst tensor which is same as src_contig
  torch::Tensor dst = torch::empty_like(src_contig);

  auto total = src_contig.numel();
  if (!total)
    return dst;

  constexpr int BLOCK = 256;
  int grid = (total + BLOCK - 1) / BLOCK;

  AT_DISPATCH_FLOATING_TYPES_AND_HALF(
      src_contig.scalar_type(), "relu_generic", [&] {
        cuda_operator::relu_generic_kernel<<<grid, BLOCK>>>(
            src_contig.data_ptr<scalar_t>(), dst.data_ptr<scalar_t>(),
            static_cast<int>(total));
      });

  C10_CUDA_KERNEL_LAUNCH_CHECK();

  return dst;
}

#endif
