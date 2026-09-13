#pragma once
#ifndef _GPU_CUH_
#define _GPU_CUH_
#include <cp_async_copy.cuh>
#include <cuda.h>
#include <cuda_runtime.h>
#include <gtest/gtest.h>
#include <tuple>
#include <vector>

inline void check(cudaError_t status) { ASSERT_EQ(status, cudaSuccess); }

inline void check_device() {
  try {
    int device = 0;
    check(cudaGetDevice(&device));
    cudaDeviceProp prop{};
    check(cudaGetDeviceProperties(&prop, device));
    if (prop.major < 8)
      throw std::runtime_error("This test targets SM80+ hardware");

  } catch (const std::exception &error) {
    std::fprintf(stderr, "FAIL: %s\n", error.what());
  }
}

// RAII
struct DeviceBuffer {
  DeviceBuffer(const DeviceBuffer &) = delete;
  DeviceBuffer &operator=(const DeviceBuffer &) = delete;

  float *ptr = nullptr;
  explicit DeviceBuffer(std::size_t n) {
    check(cudaMalloc(&ptr, std::max<std::size_t>(n, 1) * sizeof(float)));
  }
  ~DeviceBuffer() {
    if (ptr)
      cudaFree(ptr);
  }
};

#endif //_GPU_CUH_
