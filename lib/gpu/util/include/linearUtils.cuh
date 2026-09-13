#pragma once
#ifndef _LINEAR_UTILS_CUH_
#define _LINEAR_UTILS_CUH_
#include <cuda.h>
#include <cuda_runtime.h>
#include <type_traits>

namespace cudahelper {
namespace util {
__device__ std::size_t global_linear_stride();
__device__ std::size_t global_linear_tid();

template <typename T>
struct enable_atomic_if_integral
    : std::bool_constant<std::is_same_v<T, int> ||
                         std::is_same_v<T, unsigned int> ||
                         std::is_same_v<T, unsigned long long>> {};

template <class T, class Op>
__device__ T compare_and_exchange(T *address, Op &&op, T value) {

  if constexpr (enable_atomic_if_integral<T>::value) {

    T old = *address;
    T assumed;

    do {
      assumed = old;
      old = atomicCAS(address, assumed, op(assumed, value));
    } while (assumed != old);

    return old;

  } else if constexpr (std::is_same_v<T, float>) {

    auto *bits = reinterpret_cast<unsigned int *>(address);

    unsigned int old = *bits;
    unsigned int expected;

    do {
      expected = old;

      const float updated = op(__uint_as_float(expected), value);

      old = atomicCAS(bits, expected, __float_as_uint(updated));

    } while (old != expected);

    return __uint_as_float(old);

  } else if constexpr (std::is_same_v<T, double>) {

    auto *bits = reinterpret_cast<unsigned long long *>(address);

    unsigned long long old = *bits;
    unsigned long long expected;

    do {
      expected = old;

      const double updated = op(__longlong_as_double(expected), value);

      old = atomicCAS(bits, expected, __double_as_longlong(updated));

    } while (old != expected);

    return __longlong_as_double(old);

  } else {

    static_assert(std::is_same_v<T, float> || std::is_same_v<T, double> ||
                      enable_atomic_if_integral<T>::value,
                  "Unsupported type for atomic compare-and-exchange");
  }
}
} // namespace util
} // namespace cudahelper

#endif //_LINEAR_UTILS_CUH_
