#pragma once
#ifndef _ROOTGRID_HPP_
#define _ROOTGRID_HPP_
#include <cstdint>
#include <iostream>
#include <optional>
#include <utility>

namespace sparse {
namespace details {
using Coord2D = std::pair<std::intptr_t, std::intptr_t>;
} // namespace details

template <typename _Ty, typename _Layout> struct RootGrid {

  std::optional<_Ty> read(std::intptr_t x, std::intptr_t y) const {
    return _read(m_root, x, y);
  }
  void write(std::intptr_t x, std::intptr_t y, const _Ty &value) {
    _write(m_root, x, y, value);
  }
  template <typename Func> void foreach (const Func &func) {
    _foreach(m_root, 0, 0, func);
  }

private:
  template <typename Node>
  static std::optional<_Ty> _read(const Node &node, std::intptr_t x,
                                  std::intptr_t y) {
    // if it is denseblock, then it should be the last node point!
    if constexpr (Node::is_leaf) {
      auto opt = node.read(x, y);
      if (!opt.has_value())
        return std::nullopt;
      return opt->get();
    } else {
      const auto [new_x, new_y] = generateCoord<Node>(x, y);
      /*if it is hash/pointer block, then find the next block and do
       * recursion!*/
      auto next_node = node.read(new_x, new_y);

      if (!next_node.has_value())
        return std::nullopt;
      // you have to pass global coordinates to the function!
      return _read(next_node->get(), x, y);
    }
  }

  template <typename Node>
  static void _write(Node &node, std::intptr_t x, std::intptr_t y,
                     const _Ty &value) {
    // if it is denseblock, then it should be the last node point!
    if constexpr (Node::is_leaf) {
      node.write(x, y, value);
    } else {
      const auto [new_x, new_y] = generateCoord<Node>(x, y);
      /*if it is hash/pointer block, then find the next block and do
       * recursion!*/
      auto &next_node = node.touch_pointer(new_x, new_y).get();
      _write(next_node, x, y, value);
    }
  }

  template <typename Node, typename Func>
  static void _foreach(Node &node, std::intptr_t xBase, std::intptr_t yBase,
                       const Func &func) {
    if constexpr (Node::is_leaf) {
      node.foreach ([xBase, yBase, func](auto &x, auto &y, auto &value) {
        func(xBase + x, yBase + y, value);
      });
    } else {
      node.foreach ([&](auto &x, auto &y, auto &value) {
        _foreach(value, xBase + (x << Node::subblock_shift_bits),
                 yBase + (y << Node::subblock_shift_bits), func);
      });
    }
  }

  _Layout m_root; // maybe start from hashmap

  template <typename Node>
  static details::Coord2D generateCoord(const std::intptr_t x,
                                        const std::intptr_t y) {
    return std::make_pair(x >> Node::subblock_shift_bits,
                          y >> Node::subblock_shift_bits);
  }
};
} // namespace sparse

#endif //_ROOTGRID_HPP_
