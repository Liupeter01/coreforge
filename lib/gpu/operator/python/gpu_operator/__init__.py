# python/gpu_operator/__init__.py
from gpu_operator._C import relu_f32, relu_generic

__version__ = "0.0.1"
__all__ = ["relu_f32", "relu_generic"]
