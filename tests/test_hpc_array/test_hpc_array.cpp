#include <HPCHighDimensionFlatArray.hpp>

#include <cstdint>

int main() {
  hpc::HPCHighDimensionFlatArray<2, float> matrix(3, 5);
  for (std::intptr_t y = 0; y < 3; ++y) {
    for (std::intptr_t x = 0; x < 5; ++x) {
      matrix(y, x) = 42.0F;
    }
  }

  matrix.zero();
  for (std::intptr_t y = 0; y < 3; ++y) {
    for (std::intptr_t x = 0; x < 5; ++x) {
      if (matrix(y, x) != 0.0F) {
        return 1;
      }
    }
  }

  // Three ints require 12 bytes, so a 64-byte allocator must round the raw
  // allocation size while preserving the vector's logical element count.
  hpc::HPCHighDimensionFlatArray<1, int, 0, 0, 64> odd_sized(3);
  if (reinterpret_cast<std::uintptr_t>(odd_sized.data()) % 64 != 0) {
    return 2;
  }
  odd_sized(2) = 7;
  odd_sized.zero();
  if (odd_sized(2) != 0) {
    return 3;
  }

  return 0;
}
