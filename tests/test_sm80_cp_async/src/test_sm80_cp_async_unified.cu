#include <algorithm>
#include <cp_async_copy.cuh>
#include <gpu.cuh>
#include <gtest/gtest.h>

class Unified
    : public ::testing::TestWithParam<std::tuple<size_t, size_t, size_t>> {};

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

  if (m && n) {
    gemm_unified_double_buffer<<<dim3((n + 15) / 16, (m + 15) / 16),
                                 dim3(16, 16)>>>(da.ptr, db.ptr, dc.ptr, m, n,
                                                 k);
    check(cudaGetLastError());
    check(cudaDeviceSynchronize());
    check(cudaMemcpy(c.data(), dc.ptr, c.size() * sizeof(float),
                     cudaMemcpyDeviceToHost));
  }

  for (int col = 0; col < n; ++col) {
    double expected = 0;
    for (int kk = 0; kk < k; ++kk)
      expected +=
          double(a[std::size_t(row) * k + kk]) * b[std::size_t(kk) * n + col];

    const float actual = c[std::size_t(row) * n + col];

    ASSERT_EQ(expected, actual);
    if (!std::isfinite(actual) ||
        std::abs(actual - expected) > 1e-4 * (1 + std::abs(expected)))
      throw std::runtime_error("GEMM mismatch");
  }
}

INSTANTIATE_TEST_SUITE_P(SM80CPAsyncTest, Unified,
                         ::testing::Values({0, 17, 16}, {17, 0, 16}, {1, 1, 0},
                                           {17, 19, 0}, {1, 1, 1}, {15, 17, 1},
                                           {16, 16, 16}, {17, 19, 17},
                                           {31, 33, 31}, {33, 31, 32},
                                           {17, 33, 33}, {31, 19, 47},
                                           {33, 17, 48}, {37, 35, 65}));
