#include <common.cuh>

// ============================================================
// Shared + bank conflict solved
// ============================================================

TEST_P(TestStallShortTestSuite, SolvedConflict) {
  const auto [nx, ny] = GetParam();

  const std::size_t size = static_cast<std::size_t>(nx) * ny;

  std::vector<int, CudaAllocator<int, CudaMemManaged>> in(size);

  std::vector<int, CudaAllocator<int, CudaMemManaged>> out(size);

  for (std::size_t i = 0; i < size; ++i) {
    in[i] = static_cast<int>(i);
  }

  const dim3 block(BlockSize, BlockSize, 1);

  const dim3 grid((nx + BlockSize - 1) / BlockSize,
                  (ny + BlockSize - 1) / BlockSize, 1);

  float ms;

  cudaEvent_t startEvent, stopEvent;
  cudaEventCreate(&startEvent);
  cudaEventCreate(&stopEvent);

  cudaEventRecord(startEvent, 0);

  printf("Current Test is: parallel_transpose_solv_conflict [%d x %d]\n", nx,
         ny);

  nvtxRangePushA("parallel_transpose_solv_conflict");

  parallel_transpose_solv_conflict<int, BlockSize>
      <<<grid, block>>>(out.data(), in.data(), nx, ny);

  nvtxRangePop();

  cudaEventRecord(stopEvent, 0);
  cudaEventSynchronize(stopEvent);

  cudaEventElapsedTime(&ms, startEvent, stopEvent);

  ASSERT_EQ(cudaGetLastError(), cudaSuccess);

  printf("Time for parallel_transpose_solv_conflict "
         "[%d x %d] execute (ms): %f\n",
         nx, ny, ms);

  Verify(out.data(), in.data(), nx, ny);

  cudaEventDestroy(startEvent);
  cudaEventDestroy(stopEvent);
}
