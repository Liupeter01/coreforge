#pragma once
#ifndef _CUDA_RELU_CUH_
#define _CUDA_RELU_CUH_
#include <cuda_bf16.h>
#include <cuda_fp16.h>

#if defined(USE_TORCH)
#include <torch/extension.h>
torch::Tensor relu_f32(const torch::Tensor &src);
torch::Tensor relu_generic(const torch::Tensor &src);

#endif

#endif
