#pragma once
#ifndef _BASEBLOCK_HPP_
#define _BASEBLOCK_HPP_
#include <cstdint>
#include <functional>
#include <memory>
#include <optional>
#include <type_traits>

namespace sparse {
namespace details {
static constexpr inline std::intptr_t constexpr_log2(std::intptr_t n) {
  return (n < 2) ? 0 : 1 + constexpr_log2(n >> 1);
}

template <typename _Ty, typename = void> struct has_bshift : std::false_type {};

template <typename _Ty>
struct has_bshift<_Ty, std::void_t<decltype(_Ty::BShift)>>
    : std::is_integral<decltype(_Ty::BShift)> {};

template <typename _Ty, typename = void> struct get_bshift;

template <typename _Ty>
struct get_bshift<_Ty, std::enable_if_t<has_bshift<_Ty>::value, void>> {
  static constexpr std::intptr_t value = _Ty::BShift;
};
} // namespace details

template <typename SubBlock> struct SubBlockInfo {
  static_assert(details::has_bshift<SubBlock>::value,
                "SubBlockInfo: SubBlock must define static constexpr BShift");

  /*
   * we have to know the shift parameter of the lower level block
   * so we use :: to get access to the bshift data
   */
  static constexpr std::intptr_t offset_bits =
      details::get_bshift<SubBlock>::value;
};

template <std::intptr_t BlockSize, bool IsLeaf> struct BlockTraits {
  static_assert(BlockSize > 0 && ((BlockSize & (BlockSize - 1)) == 0),
                "BlockSize must be a power of 2 and > 0");

  static constexpr std::intptr_t B = BlockSize;
  static constexpr std::intptr_t BShift = details::constexpr_log2(B);
  static constexpr std::intptr_t BMask = B - 1;
  static constexpr bool is_leaf = IsLeaf;
};

template <std::intptr_t BlockSize, bool IsLeaf, typename _Ty>
struct BlockInfo : BlockTraits<BlockSize, IsLeaf> {

  using value_type = _Ty;
  using reference = _Ty &;
  using const_value = const _Ty;

  virtual std::optional<std::reference_wrapper<value_type>>
  operator()(const std::intptr_t x, const std::intptr_t y) = 0;
  virtual std::optional<std::reference_wrapper<const_value>>
  operator()(const std::intptr_t x, const std::intptr_t y) const = 0;

  virtual std::optional<std::reference_wrapper<const_value>>
  read(const std::intptr_t x, const std::intptr_t y) const = 0;

  virtual void write(const std::intptr_t x, const std::intptr_t y,
                     const _Ty &value) = 0;
  virtual void write(const std::intptr_t x, const std::intptr_t y,
                     _Ty &&value) = 0;

  virtual std::optional<std::reference_wrapper<value_type>>
  fetch_pointer(const std::intptr_t x, const std::intptr_t y) = 0;
  virtual std::reference_wrapper<value_type>
  touch_pointer(const std::intptr_t x, const std::intptr_t y) = 0;
};
} // namespace sparse

#endif // _BASEBLOCK_HPP_
