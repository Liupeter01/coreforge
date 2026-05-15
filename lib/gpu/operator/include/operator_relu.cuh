#pragma once
#ifndef _OPERATOR_RELU_CUH_
#define _OPERATOR_RELU_CUH_
#include <c10/cuda/CUDAGuard.h> //multi GPU support
#include <cuda_relu.cuh>
#include <pybind11/pybind11.h>
#include <torch/extension.h>
#include <torch/types.h>

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

  cuda_operator::relu_fp32_kernel<<<grid, BLOCK_SIZE>>>(
      src_contig.data_ptr<float>(), dst.data_ptr<float>(),
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
      src_contig.scalar_type(), "relu_generic", ([&] {
        cuda_operator::relu_generic_kernel<<<grid, BLOCK>>>(
            src_contig.data_ptr<scalar_t>(), dst.data_ptr<scalar_t>(),
            static_cast<int>(total))
      }));

  C10_CUDA_KERNEL_LAUNCH_CHECK();

  return dst;
}

PYBIND11_MODULE(TORCH_EXTENSION_NAME, m) {
  m.doc() = "My GPU Operators - Custom CUDA kernels for PyTorch";

  m.def("relu_f32", &relu_f32, "ReLU activation for Float32 tensors (CUDA)",
        py::arg("src"));

  m.def("relu", &relu_generic, "ReLU activation for all floating types (CUDA)",
        py::arg("src"));
}

#endif
