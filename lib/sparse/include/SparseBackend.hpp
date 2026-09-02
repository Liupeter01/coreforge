#pragma once

#ifndef LIBHPC_USE_TBB
#define LIBHPC_USE_TBB 0
#endif

#if LIBHPC_USE_TBB

#include <tbb/concurrent_vector.h>
#include <tbb/parallel_for.h>

#else

#include <deque>
#include <functional>

#endif

#include <cstddef>
#include <utility>

namespace sparse::details {

#if LIBHPC_USE_TBB

template <typename T> using ConcurrentSequence = tbb::concurrent_vector<T>;

template <typename Index, typename Function>
void parallel_for(Index begin, Index end, Function &&function) {
  tbb::parallel_for(begin, end, std::forward<Function>(function));
}

#else

// std::deque is intentionally used instead of std::vector: vector<bool>
// returns proxy references and is incompatible with DenseBlock<bool>.
template <typename T> using ConcurrentSequence = std::deque<T>;

template <typename Index, typename Function>
void parallel_for(Index begin, Index end, Function &&function) {
  for (Index index = begin; index < end; ++index) {
    std::invoke(function, index);
  }
}

#endif

} // namespace sparse::details
