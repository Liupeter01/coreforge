#pragma once
#ifndef _CP_ASYNC_COPY_CUH_
#define _CP_ASYNC_COPY_CUH_
#include <cooperative_groups.h>
#include <cstddef>
#include <cuda/pipeline>
#include <cuda_runtime.h>

#pragma nv_diag_suppress static_var_with_dynamic_init

using BlockPipe = cuda::pipeline<cuda::thread_scope_block>;

template <int TILE>
__device__ void stage_gemm_tile(const float *A, const float *B, float *As,
                                float *Bs, int row, int col, int batch, int M,
                                int N, int K, BlockPipe &pipe) {
  const int tx = threadIdx.x;
  const int ty = threadIdx.y;
  const int ak = batch * TILE + tx;
  const int bk = batch * TILE + ty;
  const int local = ty * TILE + tx;
  // single thread overload
  if (row < M && ak < K) {
    cuda::memcpy_async(As + local, A + std::size_t(row) * K + ak,
                       cuda::aligned_size_t<4>(sizeof(float)), pipe);
  } else {
    As[local] = 0.0f;
  }
  if (bk < K && col < N) {
    cuda::memcpy_async(Bs + local, B + std::size_t(bk) * N + col,
                       cuda::aligned_size_t<4>(sizeof(float)), pipe);
  } else {
    Bs[local] = 0.0f;
  }
}

template <int TILE = 16>
__global__ void gemm_unified_double_buffer(const float *A, const float *B,
                                           float *C, int M, int N, int K) {
  static_assert(TILE == 16, "Teaching example requires block=(16,16)");
  if (M <= 0 || N <= 0)
    return;

  __shared__ float As[2][TILE][TILE];
  __shared__ float Bs[2][TILE][TILE];

  __shared__ cuda::pipeline_shared_state<cuda::thread_scope_block, 2> state;

  auto block = cooperative_groups::this_thread_block();
  auto pipe = cuda::make_pipeline(block, &state); // unified

  const int tx = threadIdx.x;
  const int ty = threadIdx.y;
  const int row = blockIdx.y * TILE + ty;
  const int col = blockIdx.x * TILE + tx;
  const int batches = K / TILE + (K % TILE != 0); // prevent overflow
  float acc = 0.0f;

  if (batches > 0) {
    pipe.producer_acquire();
    stage_gemm_tile<TILE>(A, B, &As[0][0][0], &Bs[0][0][0], row, col, 0, M, N,
                          K, pipe);

    pipe.producer_commit();
  }

  for (int batch = 0; batch < batches; ++batch) {
    const int read_slot = batch % 2;
    if (batch + 1 < batches) {
      const int write_slot = (batch + 1) % 2;
      pipe.producer_acquire();
      stage_gemm_tile<TILE>(A, B, &As[write_slot][0][0], &Bs[write_slot][0][0],
                            row, col, batch + 1, M, N, K, pipe);

      pipe.producer_commit();
    }

    pipe.consumer_wait(); // wait for the oldest unconsumed slot to be ready
    __syncthreads();      // sync to all threads in the block before consuming

    for (int kk = 0; kk < TILE; ++kk)
      acc += As[read_slot][ty][kk] * Bs[read_slot][kk][tx];

    pipe.consumer_release();
  }

  if (row < M && col < N)
    C[std::size_t(row) * N + col] = acc;
}

__global__ void scale_partitioned_double_buffer(const float* in, float* out,
          std::size_t n);

#endif // _CP_ASYNC_COPY_CUH_
