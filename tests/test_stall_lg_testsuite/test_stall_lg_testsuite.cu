#include <cudaAllocator.hpp>
#include <cuda_tut_stall_lg.cuh>
#include <gtest/gtest.h>
#include <vector>

int main() {
  constexpr int Grid = 64, Block = 256;
  const std::size_t bytes = std::size_t(Grid) * Block * 2000;
  std::vector<int8_t, CudaAllocator<int8_t, CudaMemManaged>> stall_in(bytes);
  std::vector<int8_t, CudaAllocator<int8_t, CudaMemManaged>> stall_out(bytes);
  for (std::size_t i = 0; i < bytes; ++i)
    stall_in[i] = static_cast<int8_t>(1 + (i * 17 + i / 97) % 127);

  std::fill(stall_out.begin(), stall_out.end(), int8_t{0});

  auto check_copy = [&]() {
    auto err = cudaGetLastError();
    if (err != cudaSuccess)
      throw std::runtime_error(cudaGetErrorString(err));
    err = cudaDeviceSynchronize(); // read array from gpu, we need this
    if (err != cudaSuccess)
      throw std::runtime_error(cudaGetErrorString(err));
    for (std::size_t i = 0; i < bytes; ++i) {
      ASSERT_EQ(stall_out[i], stall_in[i]);
      if (stall_out[i] != stall_in[i])
        throw std::runtime_error("copy result mismatch");
    }
    std::fill(stall_out.begin(), stall_out.end(), int8_t{0});
  };

  auto time_kernel = [&](const char *name, auto &&launch) {
    cudaEvent_t start, stop;

    cudaEventCreate(&start);
    cudaEventCreate(&stop);

    cudaEventRecord(start);

    launch();

    auto err = cudaGetLastError();
    ASSERT_EQ(err, cudaSuccess);

    cudaEventRecord(stop);

    cudaEventSynchronize(stop);

    float ms = 0.0f;
    cudaEventElapsedTime(&ms, start, stop);

    std::cout << name << ": " << ms << " ms\n";

    cudaEventDestroy(start);
    cudaEventDestroy(stop);

    check_copy();
  };

  time_kernel("stall_lg_worse", [&] {
    stall_lg_worse<<<Grid, Block>>>(stall_in.data(), stall_out.data());
  });

  time_kernel("stall_lg_coalesced_32", [&] {
    stall_lg_coalesced_32<<<Grid, Block>>>(stall_in.data(), stall_out.data());
  });

  time_kernel("stall_lg_coalesced_128", [&] {
    stall_lg_coalesced_128<<<Grid, Block>>>(stall_in.data(), stall_out.data());
  });

  time_kernel("stall_lg_coalesced_512", [&] {
    stall_lg_coalesced_512<<<Grid, Block>>>(stall_in.data(), stall_out.data());
  });

  return 0;
}
