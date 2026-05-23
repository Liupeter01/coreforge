import os
from setuptools import setup, find_packages
from torch.utils.cpp_extension import BuildExtension, CUDAExtension

# current dir
this_dir = os.path.dirname(os.path.abspath(__file__))

setup(
    name = "gpu_operator",
    version="0.1.0",
    author="Loopback404",
    description="Custom CUDA operators for PyTorch",

    cmdclass={"build_ext": BuildExtension},

    python_requires=">=3.8",
    install_requires=["torch>=2.0"],

    # python package
    packages=find_packages(where="python"),
    package_dir={"": "python"},

        # C++ / CUDA extension
ext_modules=[
    CUDAExtension(
        name="gpu_operator._C",          #  .so module
        sources=[
            os.path.join(this_dir, "src"),
        ],
        include_dirs=[
            os.path.join(this_dir, "include"),
        ],
        extra_compile_args={
            "cxx": ["-O3", "-std=c++17"],
            "nvcc": [
                "-O3",
                "-std=c++17",
                "--expt-relaxed-constexpr",
               
                "-gencode=arch=compute_80,code=sm_80",
                "-gencode=arch=compute_89,code=sm_89",
            ],
        },
    ),
],
)
