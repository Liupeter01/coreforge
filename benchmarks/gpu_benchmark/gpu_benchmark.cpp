#include <algorithm>
#include <benchmark/benchmark.h>
#include <cmath>
#include <cudaAllocator.hpp>
#include <cuda_radix_sort.cuh>
#include <libmorton/morton.h>
#include <omp.h>
#include <radix_sort_gpu.h>
#include <vector>

// static constexpr std::size_t ARRAY_SIZE = 100000000;
static constexpr std::size_t ARRAY_SIZE = 500'000'000;

// static void BM_gpu_radix_sort_v1(benchmark::State &bm) {
//   sort_test_array.clear();
//   for (auto _ : bm) {
//     ::sort::gpu::radix::details::helper::generate_random(sort_test_array,
//                                                          ARRAY_SIZE);
//     ::sort::gpu::radix::details::v1::__radix_sort_v1(sort_test_array);
//     benchmark::DoNotOptimize(sort_test_array);
//   }
// }
//
// static void BM_gpu_radix_sort_v2(benchmark::State &bm) {
//   sort_test_array.clear();
//   for (auto _ : bm) {
//     ::sort::gpu::radix::details::helper::generate_random(sort_test_array,
//                                                          ARRAY_SIZE);
//     ::sort::gpu::radix::details::v2::__radix_sort_v2(sort_test_array);
//     benchmark::DoNotOptimize(sort_test_array);
//   }
// }
//
// static void BM_gpu_radix_sort_v3(benchmark::State &bm) {
//   sort_test_array.clear();
//   for (auto _ : bm) {
//     ::sort::gpu::radix::details::helper::generate_random(sort_test_array,
//                                                          ARRAY_SIZE);
//     ::sort::gpu::radix::details::v3::__radix_sort_v3(sort_test_array);
//     benchmark::DoNotOptimize(sort_test_array);
//   }
// }
//
// static void BM_gpu_radix_sort_v4(benchmark::State &bm) {
//   sort_test_array.clear();
//   for (auto _ : bm) {
//     ::sort::gpu::radix::details::helper::generate_random(sort_test_array,
//                                                          ARRAY_SIZE);
//     ::sort::gpu::radix::details::v4::__radix_sort_v4(sort_test_array);
//     benchmark::DoNotOptimize(sort_test_array);
//   }
// }

// BENCHMARK(BM_gpu_radix_sort_v1);
// BENCHMARK(BM_gpu_radix_sort_v2);
// BENCHMARK(BM_gpu_radix_sort_v3);
// BENCHMARK(BM_gpu_radix_sort_v4);
// BENCHMARK_MAIN();

void v1() {
  std::vector<uint32_t, CudaAllocator<uint32_t, CudaMemManaged>>
      sort_test_array;
  std::cout << "Generate\n";
  ::sort::gpu::radix::details::helper::generate_random(sort_test_array,
                                                       ARRAY_SIZE);
  std::cout << "Start Kernel\n";
  ::sort::gpu::radix::details::v1::__radix_sort_v1(sort_test_array);
  sort_test_array.clear();
}

void v2() {
  std::vector<uint32_t, CudaAllocator<uint32_t, CudaMemManaged>>
      sort_test_array;
  std::cout << "Generate\n";
  ::sort::gpu::radix::details::helper::generate_random(sort_test_array,
                                                       ARRAY_SIZE);
  std::cout << "Start Kernel\n";
  ::sort::gpu::radix::details::v2::__radix_sort_v2(sort_test_array);
  sort_test_array.clear();
}

void v3() {
  std::vector<uint32_t, CudaAllocator<uint32_t, CudaMemManaged>>
      sort_test_array;
  std::cout << "Generate\n";
  ::sort::gpu::radix::details::helper::generate_random(sort_test_array,
                                                       ARRAY_SIZE);
  std::cout << "Start Kernel\n";
  ::sort::gpu::radix::details::v3::__radix_sort_v3(sort_test_array);
  sort_test_array.clear();
}

void v4() {
  std::vector<uint32_t, CudaAllocator<uint32_t, CudaMemManaged>>
      sort_test_array;
  std::cout << "Generate\n";
  ::sort::gpu::radix::details::helper::generate_random(sort_test_array,
                                                       ARRAY_SIZE);
  std::cout << "Start Kernel\n";
  ::sort::gpu::radix::details::v4::__radix_sort_v4(sort_test_array);
  std::cout << std::is_sorted(sort_test_array.begin(), sort_test_array.end())
            << std::endl;
  sort_test_array.clear();
}

void radix_sort_test() {
  ::sort::gpu::radix::details::__kernel_startup();

  // printf("\n===============v1================\n");
  // v1();
  // printf("\n===============v2================\n");
  // v2();
  // printf("\n===============v3================\n");
  // v3();
  printf("\n===============v4================\n");
  auto now = std::chrono::high_resolution_clock::now();
  v4();
  std::cout << "Time = "
            << std::chrono::duration_cast<std::chrono::milliseconds>(
                   std::chrono::high_resolution_clock::now() - now)
                   .count()
            << "ms\n";
}

int main() {

  try {
    radix_sort_test();
  } catch (const std::exception &e) {
    std::cout << "Whats going on: " << e.what() << "\n";
  }

  return 0;
}
