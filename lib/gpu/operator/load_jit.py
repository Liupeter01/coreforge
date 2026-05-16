import torch
from torch.utils.cpp_extension import load

gpu_operator_fast_jit = load(
 name="gpu_operator",
 sources=["src/cuda_relu.cu"],
 extra_include_paths=["include/"],
 extra_cuda_cflags=["-O3", "--expt-relaxed-constexpr --expt-extended-lambda"],
 verbose=True,  
)

x = torch.randn(1024, device="cuda")
y = gpu_operator_fast_jit.relu_f32(x)