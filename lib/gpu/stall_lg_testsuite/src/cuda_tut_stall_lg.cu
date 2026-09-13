#include <cudaHelper.cuh>
#include <cuda_tut_stall_lg.cuh>
#include <nvtx3/nvToolsExt.h>

__global__ void stall_lg_worse(const int8_t *__restrict__ ptr1,
                               int8_t *__restrict__ ptr2) {
  const std::size_t id = std::size_t(blockIdx.x) * blockDim.x + threadIdx.x;
  const std::size_t offset = id * 2000;
  for (int i = 0; i < 2000; ++i) {
    ptr2[offset + i] = ptr1[offset + i];
  }
}

__global__ void stall_lg_coalesced_32(const int8_t *__restrict__ ptr1,
                                      int8_t *__restrict__ ptr2) {
  const std::size_t id = std::size_t(blockIdx.x) * blockDim.x + threadIdx.x;
  const std::size_t stride = std::size_t(gridDim.x) * blockDim.x;
  for (int i = 0; i < 2000; ++i) {
    const std::size_t index = std::size_t(i) * stride + id;
    ptr2[index] = ptr1[index];
  }
}

__global__ void stall_lg_coalesced_128(const int8_t *__restrict__ ptr1,
                                       int8_t *__restrict__ ptr2) {
  const int *p1 = reinterpret_cast<const int *>(ptr1);
  int *p2 = reinterpret_cast<int *>(ptr2);
  const std::size_t id = std::size_t(blockIdx.x) * blockDim.x + threadIdx.x;
  const std::size_t stride = std::size_t(gridDim.x) * blockDim.x;
  for (int i = 0; i < 2000 / 4; ++i) {
    const std::size_t index = std::size_t(i) * stride + id;
    p2[index] = p1[index];
  }
}

__global__ void stall_lg_coalesced_512(const int8_t *__restrict__ ptr1,
                                       int8_t *__restrict__ ptr2) {
  const int4 *p1 = reinterpret_cast<const int4 *>(ptr1);
  int4 *p2 = reinterpret_cast<int4 *>(ptr2);
  const std::size_t id = std::size_t(blockIdx.x) * blockDim.x + threadIdx.x;
  const std::size_t stride = std::size_t(gridDim.x) * blockDim.x;
  for (int i = 0; i < 2000 / 16; ++i) {
    const std::size_t index = std::size_t(i) * stride + id;
    p2[index] = p1[index];
  }
}
