#include <cuda_runtime.h>
#include <cuda_tut_mio_throttle.cuh>
#include <gtest/gtest.h>
#include <nvToolsExt.h>

TEST(StallMioTest, Worse) {
  float ms;

  cudaEvent_t startEvent, stopEvent;
  cudaEventCreate(&startEvent);
  cudaEventCreate(&stopEvent);

  cudaEventRecord(startEvent, 0);

  printf("Current Test is: %s\n", "stall_mio_worse");
  nvtxRangePushA("stall_mio_worse");

  stall_mio_worse<<<1024, 32>>>();

  nvtxRangePop();

  cudaEventRecord(stopEvent, 0);
  cudaEventSynchronize(stopEvent);
  cudaEventElapsedTime(&ms, startEvent, stopEvent);

  printf("Time for %s execute (ms): %f\n", "stall_mio_worse", ms);

  cudaEventDestroy(startEvent);
  cudaEventDestroy(stopEvent);
}

TEST(StallMioTest, Better) {
  float ms;

  cudaEvent_t startEvent, stopEvent;
  cudaEventCreate(&startEvent);
  cudaEventCreate(&stopEvent);

  cudaEventRecord(startEvent, 0);

  printf("Current Test is: %s\n", "stall_mio_better");
  nvtxRangePushA("stall_mio_better");

  stall_mio_better < <<1024, 32>>();

  nvtxRangePop();

  cudaEventRecord(stopEvent, 0);
  cudaEventSynchronize(stopEvent);
  cudaEventElapsedTime(&ms, startEvent, stopEvent);

  printf("Time for %s execute (ms): %f\n", "stall_mio_better", ms);

  cudaEventDestroy(startEvent);
  cudaEventDestroy(stopEvent);
}

TEST(StallMioTest, Good) {
  float ms;

  cudaEvent_t startEvent, stopEvent;
  cudaEventCreate(&startEvent);
  cudaEventCreate(&stopEvent);

  cudaEventRecord(startEvent, 0);

  printf("Current Test is: %s\n", "stall_mio_good");
  nvtxRangePushA("stall_mio_good");

  stall_mio_good<<<1024, 32>>>();

  nvtxRangePop();

  cudaEventRecord(stopEvent, 0);
  cudaEventSynchronize(stopEvent);
  cudaEventElapsedTime(&ms, startEvent, stopEvent);

  printf("Time for %s execute (ms): %f\n", "stall_mio_good", ms);

  cudaEventDestroy(startEvent);
  cudaEventDestroy(stopEvent);
}
