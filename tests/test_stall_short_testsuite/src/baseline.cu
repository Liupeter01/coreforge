#include <common.cuh>

// ============================================================
// Baseline
// ============================================================

TEST_P(TestStallShortTestSuite, Baseline) {
  const auto [nx, ny] = GetParam();

  const std::size_t size = static_cast<std::size_t>(nx) * ny;

  std::vector<int, CudaAllocator<int, CudaMemManaged>> in(size);

  std::vector<int, CudaAllocator<int, CudaMemManaged>> out(size);

  for (std::size_t i = 0; i < size; ++i) {
    in[i] = static_cast<int>(i);
  }

  float ms;

  cudaEvent_t startEvent, stopEvent;
  cudaEventCreate(&startEvent);
  cudaEventCreate(&stopEvent);

  constexpr int Threads = 1024;

  const std::size_t blocks = (size + Threads - 1) / Threads;

  cudaEventRecord(startEvent, 0);

  printf("Current Test is: parallel_transpose_baseline [%d x %d]\n", nx, ny);

  nvtxRangePushA("parallel_transpose_baseline");

  parallel_transpose<<<blocks, Threads>>>(out.data(), in.data(), nx, ny);

  nvtxRangePop();

  cudaEventRecord(stopEvent, 0);
  cudaEventSynchronize(stopEvent);

  cudaEventElapsedTime(&ms, startEvent, stopEvent);

  ASSERT_EQ(cudaGetLastError(), cudaSuccess);

  printf("Time for parallel_transpose_baseline "
         "[%d x %d] execute (ms): %f\n",
         nx, ny, ms);

  Verify(out.data(), in.data(), nx, ny);

  cudaEventDestroy(startEvent);
  cudaEventDestroy(stopEvent);
}
