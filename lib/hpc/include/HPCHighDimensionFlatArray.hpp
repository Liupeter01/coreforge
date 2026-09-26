/**
 * @file HPCHighDimensionFlatArray.hpp
 * @brief Definition of a high-performance, cache-aligned, padded, N-dimensional
 * flat array.
 */

#pragma once
#ifndef _HPC_HIGH_DIMENSION_FLAT_ARRAY_HPP_
#define _HPC_HIGH_DIMENSION_FLAT_ARRAY_HPP_

#include <AlignedAlloc.hpp>
#include <algorithm>
#include <array>
#include <cassert>
#include <vector>

namespace hpc {

/**
 * @brief A high-performance, flat N-dimensional array with padding (ghost
 * cells) and aligned memory.
 *
 * This class stores N-dimensional data using a flat, contiguous memory buffer
 * (row-major layout), optimized for HPC and SIMD workloads. It supports
 * boundary padding (ghost cells) on each side of each dimension via the
 * `Low_Bound` and `High_Bound` template parameters. Memory alignment is
 * guaranteed via a customizable allocator (default is `AlignedAllocator` with
 * configurable alignment).
 *
 * @tparam Dimension     Number of dimensions (must be > 0)
 * @tparam _Ty           Value type stored in the array (e.g. float, double,
 * int)
 * @tparam Low_Bound     Number of ghost cells (padding) on the lower side of
 * each dimension
 * @tparam High_Bound    Number of ghost cells (padding) on the upper side of
 * each dimension (default = Low_Bound)
 * @tparam Alignment     Memory alignment in bytes (default = 16). Should be
 * 16/32 for SSE/AVX.
 * @tparam Alloc         Custom STL-compatible allocator (default =
 * AlignedAllocator<_Ty, Alignment>)
 *
 * @note Indexing follows row-major order: the last dimension changes fastest.
 * @note The array does not perform bounds checking unless using `at()` or
 * `safe_linearize`.
 * @note Indexing assumes padded bounds: valid access indices range from
 * `-Low_Bound` to `dim[i] + High_Bound - 1`.
 *
 * @example
 * @code
 * HPCHighDimensionFlatArray<2, float, 1> array(64, 64);  // 2D array with 1
 * ghost cell padding array(0, 0) = 1.0f; float val = array.at({-1, 0});  //
 * safe access with boundary check
 * @endcode
 */
template <std::size_t Dimension, typename _Ty, std::size_t Low_Bound = 0,
          std::size_t High_Bound = Low_Bound, std::size_t Alignment = 16,
          class Alloc = AlignedAllocator<_Ty, Alignment>>
class HPCHighDimensionFlatArray {

  static_assert(Dimension > 0, "Dimension must larger than zero");
  static_assert(
      std::is_same_v<std::remove_cv_t<std::remove_reference_t<_Ty>>, _Ty>,
      "_Ty must not be cvref");
  static_assert(!std::is_same_v<_Ty, bool>,
                "vector<bool> is not a flat array of bool objects");

  // index limit should stay inside numeric_limits<>::max, in order to avoid
  // overflow after padding
  static constexpr std::size_t index_limit =
      static_cast<std::size_t>((std::numeric_limits<std::ptrdiff_t>::max)());
  static_assert(Low_Bound <= index_limit, "Low_Bound is too large");
  static_assert(High_Bound <= index_limit - Low_Bound, "Padding is too large");

  template <class I> static std::size_t checked_dimension(I value) {
    static_assert(std::is_integral_v<I> && !std::is_same_v<I, bool>,
                  "Dimensions must be integers, excluding bool");
    if constexpr (std::is_signed_v<I>) {
      if (value <= 0)
        throw std::invalid_argument("Dimension must be positive");
    } else {
      if (value == 0)
        throw std::invalid_argument("Dimension must be positive");
    }
    if (static_cast<std::uintmax_t>(value) > index_limit)
      throw std::length_error("Dimension cannot fit the index type");
    return static_cast<std::size_t>(value);
  }

public:
  /// Private constructor used by delegating constructors.
  explicit HPCHighDimensionFlatArray(
      const std::array<std::size_t, Dimension> &dims) {
    resize(dims);
  }

  /**
   * @brief Constructs the array with given per-dimension sizes.
   *
   * @tparam DimForEachLayer Variadic dimension sizes, must match Dimension.
   * @param dims Sizes for each logical dimension (excluding padding).
   */
  template <typename... DimForEachLayer,
            std::enable_if_t<
                (sizeof...(DimForEachLayer) == Dimension &&
                 std::conjunction_v<std::is_integral<DimForEachLayer>...>),
                int> = 0>
  explicit HPCHighDimensionFlatArray(const DimForEachLayer &...dims)
      : HPCHighDimensionFlatArray(
            std::array<std::size_t, Dimension>{checked_dimension(dims)...}) {}

  std::size_t size() const noexcept { return _flat.size(); } // 包含 padding

  /**
   * @brief Shrinks internal vector capacity to fit its size.
   */
  void shrink_to_fit() { _flat.shrink_to_fit(); }

  /**
   * @brief Returns a mutable pointer to the underlying flat buffer.
   */
  constexpr _Ty *data() noexcept { return _flat.data(); }

  /**
   * @brief Returns a const pointer to the underlying flat buffer.
   */
  constexpr const _Ty *data() const noexcept { return _flat.data(); }

  void zero() { std::fill(_flat.begin(), _flat.end(), _Ty{}); }

  /**
   * @brief Accesses an element using a dimension array with bounds checking.
   *
   * @param indices Array of indices (with possible ghost cell range).
   * @return Reference to the element at the given location.
   * @throws std::out_of_range if any index is outside padded bounds.
   */
  _Ty &at(const std::array<std::ptrdiff_t, Dimension> &indices) {
    return _flat[safe_linearize(indices)];
  }
  const _Ty &at(const std::array<std::ptrdiff_t, Dimension> &indices) const {
    return _flat[safe_linearize(indices)];
  }

  /**
   * @brief Direct element access without bounds checking.
   * Preconditions: every index is representable and inside its padded range.
   * @tparam Indicies Variadic integral indices (must match Dimension).
   * @param idxs Indices per dimension.
   * @return Reference to the element at the given location.
   */
  template <
      typename... Indices,
      std::enable_if_t<sizeof...(Indices) == Dimension &&
                           std::conjunction_v<std::is_integral<Indices>...>,
                       int> = 0>
  _Ty &operator()(const Indices &...indices) noexcept {
    return _flat[unsafe_linearize({static_cast<std::ptrdiff_t>(indices)...})];
  }
  template <
      typename... Indices,
      std::enable_if_t<sizeof...(Indices) == Dimension &&
                           std::conjunction_v<std::is_integral<Indices>...>,
                       int> = 0>
  const _Ty &operator()(const Indices &...indices) const noexcept {
    return _flat[unsafe_linearize({static_cast<std::ptrdiff_t>(indices)...})];
  }

protected:
  /**
   * @brief Resizes the array using a dimension array, preserving ghost cell
   * configuration.
   *
   * @param dim Dimension sizes per axis.
   * @param value Initial value to fill the flat buffer.
   */
  void resize(const std::array<std::size_t, Dimension> &dims,
              const _Ty &value = _Ty{}) {
    const auto [stride, total] = compute_stride_and_total(dims);
    std::vector<_Ty, Alloc> next(total, value, _flat.get_allocator());
    _flat.swap(next);
    _dim = dims;
    _stride = stride;
  }

  /**
   * @brief Applies ghost cell offset to an input index.
   * @param val Logical index (can be negative for ghost cells).
   * @return Internal flat index (with offset).
   */
  std::ptrdiff_t padded_index(std::ptrdiff_t value) const noexcept {
    return value + static_cast<std::ptrdiff_t>(Low_Bound);
  }

  /**
   * @brief Computes stride and total number of flat cells (with padding).
   *
   * @param dims Dimension sizes.
   * @return A pair containing the stride array and total element count.
   */
  static auto
  compute_stride_and_total(const std::array<std::size_t, Dimension> &dims)
      -> std::pair<std::array<std::size_t, Dimension>, std::size_t> {
    std::array<std::size_t, Dimension> stride{};
    std::size_t total = 1;
    for (std::size_t i = Dimension; i-- > 0;) {
      if (dims[i] == 0)
        throw std::invalid_argument("Dimension must be positive");
      if (dims[i] > index_limit - Low_Bound - High_Bound)
        throw std::length_error("Padded dimension is too large");
      const auto extent = dims[i] + Low_Bound + High_Bound;
      stride[i] = total;
      if (total > index_limit / extent)
        throw std::length_error("Total element count is too large");
      total *= extent;
    }
    return {stride, total};
  }

  /**
   * @brief Converts a multi-dimensional padded index to a flat offset (without
   * bounds check).
   *
   * @param insert Index per dimension (including ghost access).
   * @return Flat memory offset.
   */
  std::size_t unsafe_linearize(
      const std::array<std::ptrdiff_t, Dimension> &indices) const noexcept {
    std::size_t result = 0;
    for (std::size_t i = 0; i < Dimension; ++i)
      result += _stride[i] * static_cast<std::size_t>(padded_index(indices[i]));
    return result;
  }

  /**
   * @brief Converts a multi-dimensional padded index to a flat offset (with
   * bounds check).
   *
   * @param insert Index per dimension.
   * @return Flat memory offset.
   * @throws std::out_of_range if any index exceeds bounds.
   */
  std::size_t
  safe_linearize(const std::array<std::ptrdiff_t, Dimension> &indices) const {
    for (std::size_t i = 0; i < Dimension; ++i) {
      if (indices[i] < -static_cast<std::ptrdiff_t>(Low_Bound) ||
          indices[i] >= static_cast<std::ptrdiff_t>(_dim[i] + High_Bound))
        throw std::out_of_range("invalid index, out of boundary");
    }
    return unsafe_linearize(indices);
  }

private:
  /// Internal flat buffer, allocated with alignment and padding.
  std::vector<_Ty, Alloc> _flat;

  /// Logical dimensions (excluding ghost cells).
  std::array<std::size_t, Dimension> _dim{};

  /// Row-major stride for each dimension (including ghost cells).
  std::array<std::size_t, Dimension> _stride{};
};

} // namespace hpc

#endif // _HPC_HIGH_DIMENSION_FLAT_ARRAY_HPP_
