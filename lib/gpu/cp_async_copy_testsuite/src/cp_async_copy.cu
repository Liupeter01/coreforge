#include <cp_async_copy.cuh>

__global__ void scale_partitioned_double_buffer(const float* in, float* out,
          std::size_t n) {
          // blockDim=(128,1,1)£¬gridDim=(positive,1,1)£¬no overlap between in and out!!
          constexpr int Tile = 128;
          constexpr int Producers = 64;
          constexpr int Consumers = 64;

          __shared__ float buffer[2][Tile];

          __shared__ cuda::pipeline_shared_state<cuda::thread_scope_block, 2> state;
          auto block = cooperative_groups::this_thread_block();

          const int tid = threadIdx.x;

          auto pipe = cuda::make_pipeline(block, &state, cuda::std::size_t{ Producers });
          const std::size_t tiles = n / Tile + (n % Tile != 0);

          //producer only
          if (tid < Producers) {
                    std::size_t batch = 0;
                    for (std::size_t tile = blockIdx.x; tile < tiles;
                              tile += gridDim.x, ++batch) {
                              pipe.producer_acquire();
                              const int slot = batch % 2;
                              for (int j = tid; j < Tile; j += Producers) {
                                        const std::size_t index = tile * Tile + j;
                                        if (index < n) {
                                                  cuda::memcpy_async(&buffer[slot][j], in + index,
                                                            cuda::aligned_size_t<4>(sizeof(float)), pipe);
                                        }
                                        else {
                                                  buffer[slot][j] = 0.0f;
                                        }
                              }

                              pipe.producer_commit();
                    }
          }
          else
          {
                    //consumer only!
                    const int consumer_rank = tid - Producers;
                    std::size_t batch = 0;
                    for (std::size_t tile = blockIdx.x; tile < tiles;
                              tile += gridDim.x, ++batch) {
                              pipe.consumer_wait();
                              const int slot = batch % 2;
                              for (int j = consumer_rank; j < Tile; j += Consumers) {
                                        const std::size_t index = tile * Tile + j;
                                        if (index < n)
                                                  out[index] = 2.0f * buffer[slot][j];
                              }
                              pipe.consumer_release();
                    }
          }
}
