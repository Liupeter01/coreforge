#pragma once
#ifndef _ALIGNED_ALLOC_HPP_
#define _ALIGNED_ALLOC_HPP_
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <limits>
#include <new>
#include <stdexcept>
#include <type_traits>
#include <utility>

namespace hpc {
namespace detail {
inline size_t round_up_to_alignment(size_t size, size_t alignment) {
  if (size == 0) {
    return alignment;
  }

  const size_t remainder = size % alignment;
  if (remainder == 0) {
    return size;
  }

  const size_t padding = alignment - remainder;
  if (size > std::numeric_limits<size_t>::max() - padding) {
    throw std::bad_alloc();
  }
  return size + padding;
}

inline void *allocate_aligned_memory(size_t align, size_t size) {
#ifdef _MSC_VER
  return _aligned_malloc(size, align);
#else
  return std::aligned_alloc(align, round_up_to_alignment(size, align));
#endif
}
inline void deallocate_aligned_memory(void *ptr) noexcept {
#ifdef _MSC_VER
  _aligned_free(ptr);
#else
  std::free(ptr);
#endif
}
} // namespace detail

template <typename T, size_t Align = 64> class AlignedAllocator;

template <size_t Align> class AlignedAllocator<void, Align> {
  static_assert(Align >= alignof(void *),
                "Align must satisfy the platform allocation alignment");
  static_assert((Align & (Align - 1)) == 0,
                "Align must be a power of two");

public:
  typedef void *pointer;
  typedef const void *const_pointer;
  typedef void value_type;

  template <class U> struct rebind {
    typedef AlignedAllocator<U, Align> other;
  };
};

template <typename T, size_t Align> class AlignedAllocator {
  static_assert(Align >= alignof(void *),
                "Align must satisfy the platform allocation alignment");
  static_assert(Align >= alignof(T), "Align must satisfy T's alignment");
  static_assert((Align & (Align - 1)) == 0,
                "Align must be a power of two");

public:
  typedef T value_type;
  typedef T *pointer;
  typedef const T *const_pointer;
  typedef T &reference;
  typedef const T &const_reference;
  typedef size_t size_type;
  typedef ptrdiff_t difference_type;

  typedef std::true_type propagate_on_container_move_assignment;

  template <class U> struct rebind {
    typedef AlignedAllocator<U, Align> other;
  };

public:
  AlignedAllocator() noexcept {}

  template <class U>
  AlignedAllocator(const AlignedAllocator<U, Align> &) noexcept {}

  size_type max_size() const noexcept {
    return (size_type(~0) - size_type(Align)) / sizeof(T);
  }

  pointer address(reference x) const noexcept { return std::addressof(x); }

  const_pointer address(const_reference x) const noexcept {
    return std::addressof(x);
  }

  pointer allocate(size_type n,
                   typename AlignedAllocator<void, Align>::const_pointer = 0) {
    if (n > max_size()) {
      throw std::bad_array_new_length();
    }
    const size_type alignment = static_cast<size_type>(Align);
    void *ptr = detail::allocate_aligned_memory(alignment, n * sizeof(T));
    if (ptr == nullptr) {
      throw std::bad_alloc();
    }

    return reinterpret_cast<pointer>(ptr);
  }

  void deallocate(pointer p, size_type) noexcept {
    return detail::deallocate_aligned_memory(p);
  }

  template <class U, class... Args> void construct(U *p, Args &&...args) {
    ::new (reinterpret_cast<void *>(p)) U(std::forward<Args>(args)...);
  }

  void destroy(pointer p) { p->~T(); }
};

template <typename T, size_t Align> class AlignedAllocator<const T, Align> {
  static_assert(Align >= alignof(void *),
                "Align must satisfy the platform allocation alignment");
  static_assert(Align >= alignof(T), "Align must satisfy T's alignment");
  static_assert((Align & (Align - 1)) == 0,
                "Align must be a power of two");

public:
  typedef T value_type;
  typedef const T *pointer;
  typedef const T *const_pointer;
  typedef const T &reference;
  typedef const T &const_reference;
  typedef size_t size_type;
  typedef ptrdiff_t difference_type;

  typedef std::true_type propagate_on_container_move_assignment;

  template <class U> struct rebind {
    typedef AlignedAllocator<U, Align> other;
  };

public:
  AlignedAllocator() noexcept {}

  template <class U>
  AlignedAllocator(const AlignedAllocator<U, Align> &) noexcept {}

  size_type max_size() const noexcept {
    return (size_type(~0) - size_type(Align)) / sizeof(T);
  }

  const_pointer address(const_reference x) const noexcept {
    return std::addressof(x);
  }

  pointer allocate(size_type n,
                   typename AlignedAllocator<void, Align>::const_pointer = 0) {
    if (n > max_size()) {
      throw std::bad_array_new_length();
    }
    const size_type alignment = static_cast<size_type>(Align);
    void *ptr = detail::allocate_aligned_memory(alignment, n * sizeof(T));
    if (ptr == nullptr) {
      throw std::bad_alloc();
    }

    return reinterpret_cast<pointer>(ptr);
  }

  void deallocate(pointer p, size_type) noexcept {
    return detail::deallocate_aligned_memory(p);
  }

  template <class U, class... Args> void construct(U *p, Args &&...args) {
    ::new (reinterpret_cast<void *>(p)) U(std::forward<Args>(args)...);
  }

  void destroy(pointer p) { p->~T(); }
};

template <typename T, size_t TAlign, typename U, size_t UAlign>
inline bool operator==(const AlignedAllocator<T, TAlign> &,
                       const AlignedAllocator<U, UAlign> &) noexcept {
  return TAlign == UAlign;
}

template <typename T, size_t TAlign, typename U, size_t UAlign>
inline bool operator!=(const AlignedAllocator<T, TAlign> &,
                       const AlignedAllocator<U, UAlign> &) noexcept {
  return TAlign != UAlign;
}
} // namespace hpc
#endif
