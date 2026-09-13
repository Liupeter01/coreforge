#pragma once
#ifndef _COMMON_H_
#define _COMMON_H_
#include <cuda_runtime.h>
#include <gtest/gtest.h>
#include <nvtx3/nvToolsExt.h>

#include <cstddef>
#include <tuple>
#include <vector>
#include <cuda_tut_stall_short.cuh>
#include <cudaAllocator.hpp>

using TransposeParam = std::tuple<int, int>;

class TestStallShortTestSuite : public ::testing::TestWithParam<TransposeParam> {
protected:
  static constexpr int BlockSize = 32;

  static void Verify(const int *out, const int *in, int nx, int ny) {
    for (int y = 0; y < ny; ++y) {
      for (int x = 0; x < nx; ++x) {

        const std::size_t src = static_cast<std::size_t>(y) * nx + x;

        const std::size_t dst = static_cast<std::size_t>(x) * ny + y;

        ASSERT_EQ(out[dst], in[src])
            << "Transpose mismatch at "
            << "x=" << x << ", y=" << y << ", nx=" << nx << ", ny=" << ny;
      }
    }
  }
};

#endif // _COMMON_H_
