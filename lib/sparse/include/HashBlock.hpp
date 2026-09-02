#pragma once
#ifndef _HASHBLOCK_HPP_
#define _HASHBLOCK_HPP_
#include <BaseBlock.hpp>
#include <SparseBackend.hpp>
#include <mutex>
#include <shared_mutex>
#include <unordered_map>
#include <vector>

#if LIBHPC_USE_TBB
#include <tbb/concurrent_hash_map.h>
#endif

namespace sparse {
namespace details {
using Coord2D = std::pair<std::intptr_t, std::intptr_t>;
template <typename SizeT>
static void hash_combine_impl(SizeT &seed, SizeT value) {
  seed ^= value + 0x9e3779b9 + (seed << 6) + (seed >> 2);
}

struct Coord2D_HashCompare {
  static size_t hash(const Coord2D &key) {
    std::size_t seed = 0;
    sparse::details::hash_combine_impl(seed,
                                       static_cast<std::size_t>(key.first));
    sparse::details::hash_combine_impl(seed,
                                       static_cast<std::size_t>(key.second));
    return seed;
  }
  static bool equal(const details::Coord2D &a, const details::Coord2D &b) {
    return a == b;
  }

  std::size_t operator()(const details::Coord2D &key) const {
    return hash(key);
  }
};

} // namespace details

template <typename OtherBlock>
struct HashBlock
    : public BlockInfo<1, false, OtherBlock> /*only related to current block,
                                                which is hashblock*/
{
  using value_type = OtherBlock;
  using reference = OtherBlock &;
  using const_value = const OtherBlock;
  using CurrBlockType = BlockInfo<1, false, OtherBlock>;

#if LIBHPC_USE_TBB
  using ContainerType =
      tbb::concurrent_hash_map<details::Coord2D, std::unique_ptr<OtherBlock>,
                               details::Coord2D_HashCompare>;

  using ContainerAccessor = typename ContainerType::accessor;
  using ConstContainerAccessor = typename ContainerType::const_accessor;
#else
  using ContainerType =
      std::unordered_map<details::Coord2D, std::unique_ptr<OtherBlock>,
                         details::Coord2D_HashCompare>;
#endif

  /*related to subblock, we need its offset argument*/
  static constexpr std::intptr_t subblock_shift_bits =
      SubBlockInfo<OtherBlock>::offset_bits;

  virtual std::optional<std::reference_wrapper<value_type>>
  operator()(const std::intptr_t x, const std::intptr_t y) override {
    auto key = getKey(x, y);
#if LIBHPC_USE_TBB
    ContainerAccessor accessor;
    if (!m_data.find(accessor, key)) {
      return std::nullopt;
    }
    return std::ref(*accessor->second); // accessor acts like iterator
#else
    std::shared_lock lock(m_mutex);
    auto it = m_data.find(key);
    if (it == m_data.end()) {
      return std::nullopt;
    }
    return std::ref(*it->second);
#endif
  }

  virtual std::optional<std::reference_wrapper<const_value>>
  operator()(const std::intptr_t x, const std::intptr_t y) const override {
    auto key = getKey(x, y);
#if LIBHPC_USE_TBB
    ConstContainerAccessor accessor;
    if (!m_data.find(accessor, key)) {
      return std::nullopt;
    }
    return std::cref(*accessor->second);
#else
    std::shared_lock lock(m_mutex);
    auto it = m_data.find(key);
    if (it == m_data.end()) {
      return std::nullopt;
    }
    return std::cref(*it->second);
#endif
  }

  virtual std::optional<std::reference_wrapper<const_value>>
  read(const std::intptr_t x, const std::intptr_t y) const override {
    return operator()(x, y);
  }

  virtual void write(const std::intptr_t x, const std::intptr_t y,
                     const OtherBlock &value) override {
    touch_pointer(x, y).get() = value;
  }
  virtual void write(const std::intptr_t x, const std::intptr_t y,
                     OtherBlock &&value) override {
    touch_pointer(x, y).get() = std::move(value);
  }

  virtual std::optional<std::reference_wrapper<value_type>>
  fetch_pointer(const std::intptr_t x, const std::intptr_t y) override {
    return operator()(x, y);
  }

  virtual std::reference_wrapper<value_type>
  touch_pointer(const std::intptr_t x, const std::intptr_t y) override {
    auto key = getKey(x, y);
#if LIBHPC_USE_TBB
    ContainerAccessor accessor;
    m_data.insert(accessor, key);

    if (!accessor->second) {
      accessor->second = std::make_unique<OtherBlock>();
    }
    return std::ref(*accessor->second);
#else
    std::unique_lock lock(m_mutex);
    auto [it, inserted] = m_data.try_emplace(key);
    if (inserted || !it->second) {
      it->second = std::make_unique<OtherBlock>();
    }
    return std::ref(*it->second);
#endif
  }

  template <typename Func> void foreach (Func &&func) {
#if LIBHPC_USE_TBB
    std::vector<details::Coord2D> keys;
    keys.reserve(m_data.size());

    for (auto it = m_data.begin(); it != m_data.end(); ++it) {
      keys.push_back(it->first);
    }
    auto size = keys.size();
    details::parallel_for(std::size_t(0), size, [&](std::size_t i) {
      ContainerAccessor acc;
      if (m_data.find(acc, keys[i])) {
        func(acc->first.first, acc->first.second, *acc->second);
      }
    });
#else
    std::vector<std::pair<details::Coord2D, OtherBlock *>> entries;
    {
      std::shared_lock lock(m_mutex);
      entries.reserve(m_data.size());
      for (auto &[key, value] : m_data) {
        entries.emplace_back(key, value.get());
      }
    }
    for (auto &[key, value] : entries) {
      func(key.first, key.second, *value);
    }
#endif
  }

  ContainerType m_data;

#if !LIBHPC_USE_TBB
  mutable std::shared_mutex m_mutex;
#endif

private:
  details::Coord2D getKey(const std::intptr_t x, const std::intptr_t y) const {
    return std::make_pair(x, y);
  }
};
} // namespace sparse

#endif // _HASHBLOCK_HPP_
