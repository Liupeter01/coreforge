#include <cuda_tut_mio_throttle.cuh>
#include <iostream>
#include <nvtx3/nvToolsExt.h>

__global__ void stall_mio_worse() {
  int id =
      threadIdx.x; // there should only 32 threads per block for this kernel
  __shared__ volatile int block1[32][32];
  __shared__ volatile int block2[32][32];

  // init data
  for (int i = 0; i < 32; ++i)
    block1[i][id] = i * 32 + id;

  __syncthreads();
  for (int i = 0; i < 32; ++i)
    block2[id][i] = block1[id][i];
  __syncthreads();
}

__global__ void stall_mio_better() {
  int id =
      threadIdx.x; // there should only 32 threads per block for this kernel
  __shared__ volatile int block1[32][32];
  __shared__ volatile int block2[32][32];

  // init data
  for (int i = 0; i < 32; ++i)
    block1[i][id] = i * 32 + id;

  __syncthreads();
  for (int i = 0; i < 32; ++i)
    block2[i][id] = block1[id][i]; // read contention
  __syncthreads();
}

__global__ void stall_mio_good() {
  int id = threadIdx.x; // there should only 32 threads per block for this
                        // kernel
  __shared__ volatile int block1[32][33]; // padding
  __shared__ volatile int block2[32][32];

  // init data
  for (int i = 0; i < 32; ++i)
    block1[i][id] = i * 32 + id;

  __syncthreads();

  for (int i = 0; i < 32; ++i)
    block2[i][id] = block1[id][i]; // stride=33
  __syncthreads();
}
