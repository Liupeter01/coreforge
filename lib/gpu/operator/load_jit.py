import torch
from torch.utils.cpp_extension import load

gpu_operator_fast_jit = load(
 name="gpu_operator",
 sources=["src/cuda_relu.cu","src/bindings.cpp"],
 extra_include_paths=["include/"],
extra_cflags=[
        "/D_HAS_STD_BYTE=0",
        "/Zc:preprocessor",
        "/wd4819",
        "/EHsc", 
        "/std:c++17",
    ],
    extra_cuda_cflags=[
        "-O3",
        "--expt-relaxed-constexpr",
        "--expt-extended-lambda",
        "-DWIN32_LEAN_AND_MEAN",
        "-D_HAS_STD_BYTE=0",
        "--std=c++17",
    ],
    verbose=True,
)

x = torch.randn(1024, device="cuda")
y = gpu_operator_fast_jit.relu_f32(x)
print(y[:10])