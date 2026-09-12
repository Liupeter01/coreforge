#include <algorithm>
#include <cp_async_copy.cuh>
#include <gpu.cuh>
#include <gtest/gtest.h>

class Partitioned
    : public ::testing::TestWithParam<
          std::pair</*grid*/ std::size_t, /*length*/ std::size_t>> {};

TEST_P(Partitioned, DifferentLengths) {
  check_device();

  auto param = GetParam();
  unsigned grid = param.first;
  size_t length = param.second;

  std::vector<float> in(length), out(length);

  for (std::size_t i = 0; i < length; ++i)
    in[i] = float(int(i % 31) - 15);

  DeviceBuffer di(length), dout(length);
  if (length) {
    check(cudaMemcpy(di.ptr, in.data(), length * sizeof(float),
                     cudaMemcpyHostToDevice));
    scale_partitioned_double_buffer<<<grid, 128>>>(di.ptr, dout.ptr, length);
    check(cudaGetLastError());
    check(cudaDeviceSynchronize());
    check(cudaMemcpy(out.data(), dout.ptr, length * sizeof(float),
                     cudaMemcpyDeviceToHost));
  }

  for (std::size_t i = 0; i < length; ++i)
    ASSERT_EQ(out[i], 2 * in[i]);
}

INSTANTIATE_TEST_SUITE_P(
    SM80CPAsyncTest, Partitioned,
    ::testing::Values({1u, 0}, {1u, 1}, {1u, 63}, {1u, 64}, {1u, 127},
                      {1u, 128}, {1u, 129}, {1u, 255}, {1u, 256}, {1u, 257},
                      {1u, 4097}, {1u, 10003},

                      {3u, 0}, {3u, 1}, {3u, 63}, {3u, 64}, {3u, 127},
                      {3u, 128}, {3u, 129}, {3u, 255}, {3u, 256}, {3u, 257},
                      {3u, 4097}, {3u, 10003},

                      {32u, 0}, {32u, 1}, {32u, 63}, {32u, 64}, {32u, 127},
                      {32u, 128}, {32u, 129}, {32u, 255}, {32u, 256},
                      {32u, 257}, {32u, 4097}, {32u, 10003}));
