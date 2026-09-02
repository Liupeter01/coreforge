#pragma once
#ifndef _DENSEBLOCK_HPP_
#define _DENSEBLOCK_HPP_
#include <BaseBlock.hpp>
#include <SparseBackend.hpp>
#include <cassert>

namespace sparse {
namespace details {
using Coord2D = std::pair<std::intptr_t, std::intptr_t>;
} // namespace details
template <std::intptr_t BlockSize, typename _Ty>
struct DenseBlock : BlockInfo<BlockSize, true, _Ty> {

  static_assert((BlockSize & (BlockSize - 1)) == 0,
                "BlockSize must be a power of 2");

  using value_type = _Ty;
  using reference = _Ty &;
  using const_value = const _Ty;

  DenseBlock() { m_block.resize(BlockSize * BlockSize); }

  std::optional<std::reference_wrapper<value_type>>
  operator()(const std::intptr_t x, const std::intptr_t y) override {
    auto [new_x, new_y] = getTransferredCoord(x, y);
    return std::make_optional(std::ref(m_block[new_x * BlockSize + new_y]));
  }

  std::optional<std::reference_wrapper<const_value>>
  operator()(const std::intptr_t x, const std::intptr_t y) const override {
    auto [new_x, new_y] = getTransferredCoord(x, y);
    return std::make_optional(std::cref(m_block[new_x * BlockSize + new_y]));
  }

  std::optional<std::reference_wrapper<const_value>>
  read(const std::intptr_t x, const std::intptr_t y) const override {
    return operator()(x, y);
  }

  void write(const std::intptr_t x, const std::intptr_t y,
             const _Ty &value) override {
    touch_pointer(x, y).get() = value;
  }

  void write(const std::intptr_t x, const std::intptr_t y,
             _Ty &&value) override {
    touch_pointer(x, y).get() = std::move(value);
  }

  std::optional<std::reference_wrapper<value_type>>
  fetch_pointer(const std::intptr_t x, const std::intptr_t y) override {
    return operator()(x, y);
  }

  std::reference_wrapper<value_type>
  touch_pointer(const std::intptr_t x, const std::intptr_t y) override {
    auto opt = fetch_pointer(x, y);
    assert(opt.has_value());
    return *opt;
  }

  template <typename Func> void foreach (Func &&func) {
#pragma omp parallel for collapse(2)
    for (long long x = 0; x < BlockSize; ++x) {
      for (long long y = 0; y < BlockSize; ++y) {
        func(x, y, m_block[x * BlockSize + y]);
      }
    }
  }

  details::ConcurrentSequence<_Ty> m_block;

private:
  details::Coord2D getTransferredCoord(const std::intptr_t x,
                                       const std::intptr_t y) const {
    return std::make_pair(x & this->BMask, y & this->BMask);
  }
};
} // namespace sparse

#endif //_DENSEBLOCK_HPP_
