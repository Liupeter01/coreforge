#include <cuda_relu.cuh>

PYBIND11_MODULE(TORCH_EXTENSION_NAME, m) {
          m.doc() = "My GPU Operators - Custom CUDA kernels for PyTorch";

          m.def("relu_f32", &relu_f32, "ReLU activation for Float32 tensors (CUDA)",
                    py::arg("src"));

          m.def("relu", &relu_generic, "ReLU activation for all floating types (CUDA)",
                    py::arg("src"));
}