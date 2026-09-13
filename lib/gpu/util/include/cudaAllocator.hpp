#pragma once
#ifndef _CUDA_ALLOCATOR_HPP_
#define _CUDA_ALLOCATOR_HPP_
#include <cuda_runtime.h>
#include <numeric>
#include <stdexcept>
#include <type_traits>
#include <cudaHelper.cuh>

struct CudaMemStrategy {
  virtual cudaError_t malloc(void **ptr, std::size_t size) = 0;
  virtual cudaError_t free(void *ptr) = 0;
};

struct CudaMemGPUOnly : public CudaMemStrategy {
  cudaError_t malloc(void **ptr, std::size_t size) override {
    return cudaMalloc(ptr, size);
  }
  cudaError_t free(void *ptr) override { return cudaFree(ptr); }
};

struct CudaMemManaged : public CudaMemStrategy {
  cudaError_t malloc(void **ptr, std::size_t size) override {
    return cudaMallocManaged(ptr, size);
  }
  cudaError_t free(void *ptr) override { return cudaFree(ptr); }
};

template <typename _Strategy, class = void>
struct has_malloc_and_free : std::false_type {};

template <typename _Strategy>
struct has_malloc_and_free<
    _Strategy,
    std::void_t<decltype(std::declval<std::decay_t<_Strategy>>().malloc(
                    (void **)nullptr, std::declval<std::size_t>())),
                decltype(std::declval<std::decay_t<_Strategy>>().free(
                    (void *)nullptr))>> : std::true_type {};

template <typename _Ty, typename _Strategy> struct CudaAllocator {
  static_assert(has_malloc_and_free<_Strategy>::value,
                "Strategy must provide static cudaError_t malloc(void**, "
                "size_t) and free(void*)");
  using value_type = _Ty;
  using pointer = _Ty *;
  using const_pointer = const _Ty *;
  using size_type = std::size_t;
  using difference_type = std::ptrdiff_t;

  using propagate_on_container_move_assignment = std::true_type;
  using is_always_equal = std::false_type;

  explicit CudaAllocator(_Strategy s = {}) noexcept : strat_(std::move(s)) {}

  template <class U, typename Strategy>
  constexpr CudaAllocator(const CudaAllocator<U, Strategy> &o) noexcept
      : strat_(o.strategy()) {}

  _Ty *allocate(std::size_t size) {
    if (sizeof(_Ty) > 1 &&
        size > std::numeric_limits<std::size_t>::max() / sizeof(_Ty)) {
      throw std::bad_array_new_length{};
    }
    void *ptr{nullptr};
    auto res = strat_.malloc(&ptr, size * sizeof(_Ty));
    if (res == cudaErrorMemoryAllocation) {
      throw std::bad_alloc();
    }

    return static_cast<_Ty *>(ptr);
  }
  void deallocate(_Ty *ptr, std::size_t size = 0) {
            cudahelper::checkCuda(strat_.free(ptr));
  }
  template <typename... Args> void construct(_Ty *p, Args &&...args) {
    if constexpr (!(sizeof...(args) == 0 && std::is_pod_v<_Ty>))
      new (reinterpret_cast<void *>(p)) _Ty(std::forward<Args>(args)...);
  }

  const _Strategy &strategy() const noexcept { return strat_; }

private:
  _Strategy strat_;
};

template <class T, class U, typename _Strategy,
          std::enable_if_t<has_malloc_and_free<_Strategy>::value, int> = 0>
bool operator==(const CudaAllocator<T, _Strategy> &,
                const CudaAllocator<U, _Strategy> &) {
  return true;
}

template <class T, class U, typename _Strategy,
          std::enable_if_t<has_malloc_and_free<_Strategy>::value, int> = 0>
bool operator!=(const CudaAllocator<T, _Strategy> &,
                const CudaAllocator<U, _Strategy> &) {
  return false;
}

#endif
