#pragma once
#ifndef _CUDA_RELU_CUH_
#define _CUDA_RELU_CUH_

#include <torch/types.h>
#include <torch/extension.h>

torch::Tensor relu_f32(const torch::Tensor& src);
torch::Tensor relu_generic(const torch::Tensor& src);

#endif
