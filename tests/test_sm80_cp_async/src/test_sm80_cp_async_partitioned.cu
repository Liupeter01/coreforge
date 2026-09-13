#include <algorithm>
#include <gpu.cuh>

using MyTestParam = ::std::tuple</*grid*/ std::size_t, /*length*/ std::size_t>;

class Partitioned : public ::testing::TestWithParam<MyTestParam> {};

TEST_P(Partitioned, DifferentLengths) {
  check_device();

  auto [grid, length] = GetParam();

  std::vector<float> in(length), out(length);

  for (std::size_t i = 0; i < length; ++i)
    in[i] = float(int(i % 31) - 15);

  DeviceBuffer di(length), dout(length);

  float elapsed_ms = 0.0f;

  if (length) {
    check(cudaMemcpy(di.ptr, in.data(), length * sizeof(float),
                     cudaMemcpyHostToDevice));

    cudaEvent_t start, stop;
    check(cudaEventCreate(&start));
    check(cudaEventCreate(&stop));

    check(cudaEventRecord(start));

    scale_partitioned_double_buffer<<<grid, 128>>>(di.ptr, dout.ptr, length);

    check(cudaGetLastError());

    check(cudaEventRecord(stop));
    check(cudaEventSynchronize(stop));

    check(cudaEventElapsedTime(&elapsed_ms, start, stop));

    check(cudaEventDestroy(start));
    check(cudaEventDestroy(stop));

    std::cout << "grid=" << grid << ", length=" << length << ": "
              << elapsed_ms * 1000.0f << " us\n";

    check(cudaMemcpy(out.data(), dout.ptr, length * sizeof(float),
                     cudaMemcpyDeviceToHost));
  }

  for (std::size_t i = 0; i < length; ++i)
    ASSERT_EQ(out[i], 2 * in[i]);
}

INSTANTIATE_TEST_SUITE_P(
    SM80AsyncTest, Partitioned,
    ::testing::ValuesIn(std::vector<MyTestParam>{
        {1, 0},    {1, 1},    {1, 63},   {1, 64},   {1, 127},   {1, 128},
        {1, 129},  {1, 255},  {1, 256},  {1, 257},  {1, 4097},  {1, 10003},

        {3, 0},    {3, 1},    {3, 63},   {3, 64},   {3, 127},   {3, 128},
        {3, 129},  {3, 255},  {3, 256},  {3, 257},  {3, 4097},  {3, 10003},

        {32, 0},   {32, 1},   {32, 63},  {32, 64},  {32, 127},  {32, 128},
        {32, 129}, {32, 255}, {32, 256}, {32, 257}, {32, 4097}, {32, 10003}}));
