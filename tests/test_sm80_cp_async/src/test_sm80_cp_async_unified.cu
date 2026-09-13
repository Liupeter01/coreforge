#include <algorithm>
#include <gpu.cuh>

using MyTestParam = std::tuple<size_t, size_t, size_t>;

class Unified : public ::testing::TestWithParam<MyTestParam> {};

TEST_P(Unified, DifferentShapes) {
  check_device();
  auto [m, n, k] = GetParam();

  std::vector<float> a(std::size_t(m) * k), b(std::size_t(k) * n);
  std::vector<float> c(std::size_t(m) * n, -123.0f);

  for (std::size_t i = 0; i < a.size(); ++i)
    a[i] = float(int(i % 17) - 8) / 8;

  for (std::size_t i = 0; i < b.size(); ++i)
    b[i] = float(int(i % 13) - 6) / 8;

  DeviceBuffer da(a.size()), db(b.size()), dc(c.size());

  if (!a.empty())
    check(cudaMemcpy(da.ptr, a.data(), a.size() * sizeof(float),
                     cudaMemcpyHostToDevice));

  if (!b.empty())
    check(cudaMemcpy(db.ptr, b.data(), b.size() * sizeof(float),
                     cudaMemcpyHostToDevice));

  float elapsed_ms = 0.0f;

  if (m && n) {
    cudaEvent_t start, stop;
    check(cudaEventCreate(&start));
    check(cudaEventCreate(&stop));

    dim3 grid((n + 15) / 16, (m + 15) / 16);
    dim3 block(16, 16);

    check(cudaEventRecord(start));

    gemm_unified_double_buffer<16>
        <<<grid, block>>>(da.ptr, db.ptr, dc.ptr, m, n, k);

    check(cudaGetLastError());

    check(cudaEventRecord(stop));
    check(cudaEventSynchronize(stop));

    check(cudaEventElapsedTime(&elapsed_ms, start, stop));

    check(cudaEventDestroy(start));
    check(cudaEventDestroy(stop));

    std::cout << "GEMM " << m << "x" << n << "x" << k << ": " << elapsed_ms
              << " ms\n";

    check(cudaMemcpy(c.data(), dc.ptr, c.size() * sizeof(float),
                     cudaMemcpyDeviceToHost));
  }

  for (size_t row = 0; row < m; ++row) {
    for (size_t col = 0; col < n; ++col) {
      double expected = 0;

      for (size_t kk = 0; kk < k; ++kk)
        expected += double(a[row * k + kk]) * b[kk * n + col];

      const float actual = c[row * n + col];

      ASSERT_TRUE(std::isfinite(actual));

      ASSERT_NEAR(expected, actual, 1e-4 * (1 + std::abs(expected)));
    }
  }
}

INSTANTIATE_TEST_SUITE_P(SM80AsyncTest, Unified,
                         ::testing::ValuesIn(std::vector<MyTestParam>{
                             {0, 17, 16},
                             {17, 0, 16},
                             {1, 1, 0},
                             {17, 19, 0},
                             {1, 1, 1},
                             {15, 17, 1},
                             {16, 16, 16},
                             {17, 19, 17},
                             {31, 33, 31},
                             {33, 31, 32},
                             {17, 33, 33},
                             {31, 19, 47},
                             {33, 17, 48},
                             {37, 35, 65}}));
