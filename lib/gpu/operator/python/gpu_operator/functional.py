# python/gpu_operator/functional.py
import torch
from gpu_operator._C import relu_f32 as _relu_f32_cuda
from gpu_operator._C import relu_generic as _relu_generic_cuda

def relu_generic(input: torch.Tensor) -> torch.Tensor:
    """
    Custom ReLU activation.

    Automatically dispatches to CUDA kernel when input is on GPU,
    falls back to PyTorch's native implementation on CPU.

    Args:
        input: Input tensor of any floating type.

    Returns:
        Output tensor with ReLU applied element-wise.
    """
    if not input.is_floating_point():
        raise TypeError(f"relu requires floating point input, got {input.dtype}")

    if input.is_cuda:
        return _relu_f32_cuda(input)
    else:
        # CPU fallback
        return torch.nn.functional.relu(input)
   
def relu_f32(input: torch.Tensor) -> torch.Tensor:
    """
    ReLU activation, Float32 only, CUDA only.

    Args:
        input: Float32 CUDA tensor.

    Returns:
        Output tensor with ReLU applied element-wise.

    Raises:
        RuntimeError: If input is not Float32 or not on CUDA.
    """
    if not input.is_cuda:
        raise RuntimeError("relu_f32 requires CUDA tensor")
    if input.dtype != torch.float32:
        raise RuntimeError(f"relu_f32 requires float32, got {input.dtype}")
    return _relu_f32_cuda(input)