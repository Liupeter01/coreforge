#pragma once
#ifndef _RADIX_SORT_HPP_
#define _RADIX_SORT_HPP_
#include <OpenMPCompat.hpp>
#include <memory>
#include <numeric>
#include <vector>

namespace sort {
namespace radix {
namespace details {
struct helper {
  static void write_back(std::uint32_t *a,
                         const std::vector<std::vector<uint32_t>> &bin,
                         std::size_t n);
  static void clean_bin(std::vector<std::vector<uint32_t>> &bins);

  static constexpr inline std::intptr_t constexpr_log2(std::intptr_t n) {
    return (n < 2) ? 0 : 1 + constexpr_log2(n >> 1);
  }

  static void generate_random(std::vector<uint32_t> &vec,
                              const std::size_t numbers);
};

#define RADIX_REQUIRE_ALLOWED_BINSIZE(B)                                       \
  do {                                                                         \
    static_assert((B) == 16u || (B) == 256u || (B) == 65536u ||                \
                      (B) == 4294967296ull,                                    \
                  "BinSize must be one of {0xF, 0xFF, 0xFFFF, 0xFFFFFFFF}");   \
  } while (0)

template <std::size_t BinSize = 256>
static void radix_sort_v1(std::uint32_t *a, std::size_t n) {
  RADIX_REQUIRE_ALLOWED_BINSIZE(BinSize);

  static constexpr std::size_t RSHIFT =
      details::helper::constexpr_log2(BinSize);
  static constexpr std::size_t ROUND =
      std::numeric_limits<uint32_t>::digits / RSHIFT;
  std::vector<std::vector<uint32_t>> bin(BinSize);

  for (std::size_t round = 0; round < ROUND; ++round) {
    details::helper::clean_bin(bin);
    std::size_t new_shift = RSHIFT * round;
    for (std::size_t i = 0; i < n; ++i) {
      std::uint32_t index = a[i];
      bin[(index >> new_shift) & (BinSize - 1)].push_back(index);
    }
    details::helper::write_back(a, bin, BinSize);
  }
}

template <std::size_t BinSize = 256>
static void radix_sort_v2(std::uint32_t *a, std::size_t n) {
  RADIX_REQUIRE_ALLOWED_BINSIZE(BinSize);

  static constexpr std::size_t RSHIFT =
      details::helper::constexpr_log2(BinSize);
  static constexpr std::size_t ROUND =
      std::numeric_limits<uint32_t>::digits / RSHIFT;
  std::vector<std::vector<uint32_t>> bin(BinSize);
  uint32_t counter[BinSize]{0};
  uint32_t curr[BinSize]{0};

  for (std::size_t round = 0; round < ROUND; ++round) {
    std::size_t new_shift = RSHIFT * round;
    details::helper::clean_bin(bin);
    memset(counter, 0, sizeof(uint32_t) * BinSize);
    memset(curr, 0, sizeof(uint32_t) * BinSize);

    for (std::size_t i = 0; i < n; ++i) {
      std::uint32_t index = a[i];
      counter[(index >> new_shift) & (BinSize - 1)]++;
    }

    for (std::size_t i = 0; i < BinSize; ++i)
      bin[i].resize(counter[i]);

    for (std::size_t i = 0; i < n; ++i) {
      std::uint32_t value = a[i];
      std::size_t index = (value >> new_shift) & (BinSize - 1);
      bin[index][curr[index]++] = value;
    }
    details::helper::write_back(a, bin, BinSize);
  }
}

template <std::size_t BinSize = 256>
static void radix_sort_v3(std::uint32_t *a, std::size_t n) {
  RADIX_REQUIRE_ALLOWED_BINSIZE(BinSize);

  static constexpr std::size_t RSHIFT =
      details::helper::constexpr_log2(BinSize);
  static constexpr std::size_t ROUND =
      std::numeric_limits<uint32_t>::digits / RSHIFT;
  std::vector<uint32_t> bin(n);
  uint32_t counter[BinSize]{0};
  uint32_t index[BinSize]{0};

  for (std::size_t round = 0; round < ROUND; ++round) {
    std::size_t new_shift = RSHIFT * round;
    memset(counter, 0, sizeof(uint32_t) * BinSize);
    memset(index, 0, sizeof(uint32_t) * BinSize);
    for (std::size_t i = 0; i < n; ++i) {
      std::uint32_t index = a[i];
      counter[(index >> new_shift) & (BinSize - 1)]++;
    }

    std::size_t sum = 0;
    for (std::size_t i = 0; i < 256; ++i) {
      index[i] = sum;
      sum += counter[i];
    }

    for (std::size_t i = 0; i < n; ++i) {
      std::uint32_t value = a[i];
      std::size_t offset = (value >> new_shift) & (BinSize - 1);
      bin[index[offset]++] = value;
    }

    memcpy(a, bin.data(), sizeof(uint32_t) * n);
  }
}

template <std::size_t BinSize = 256>
static void radix_sort_v4(std::uint32_t *a, std::size_t n) {

  RADIX_REQUIRE_ALLOWED_BINSIZE(BinSize);

  static constexpr std::size_t RSHIFT =
      details::helper::constexpr_log2(BinSize);
  static constexpr std::size_t ROUND =
      std::numeric_limits<uint32_t>::digits / RSHIFT;
  std::vector<uint32_t> bin(n);

  uint32_t counter[BinSize]{0};
  uint32_t index[BinSize]{0};

  uint32_t *buffer = bin.data();

  for (std::size_t round = 0; round < ROUND; ++round) {
    std::size_t new_shift = RSHIFT * round;
    memset(counter, 0, sizeof(uint32_t) * BinSize);
    memset(index, 0, sizeof(uint32_t) * BinSize);

    for (std::size_t i = 0; i < n; ++i) {
      std::uint32_t index = a[i];
      counter[(index >> new_shift) & (BinSize - 1)]++;
    }

    std::size_t sum = 0;
    for (std::size_t i = 0; i < BinSize; ++i) {
      index[i] = sum;
      sum += counter[i];
    }

    for (std::size_t i = 0; i < n; ++i) {
      std::uint32_t value = a[i];
      std::size_t offset = (value >> new_shift) & (BinSize - 1);
      buffer[index[offset]++] = value;
    }
    std::swap(a, buffer);
  }
}

template <std::size_t BinSize = 256>
static void radix_sort_cache_v1(std::uint32_t *a, std::size_t n) {
  RADIX_REQUIRE_ALLOWED_BINSIZE(BinSize);

  static constexpr std::size_t RSHIFT =
      details::helper::constexpr_log2(BinSize);
  static constexpr std::size_t ROUND =
      std::numeric_limits<uint32_t>::digits / RSHIFT;
  std::vector<uint32_t> bin(n);

  uint32_t counter[BinSize]{0};

  uint32_t *buffer = bin.data();

  for (std::size_t round = 0; round < ROUND; ++round) {
    std::size_t new_shift = RSHIFT * round;
    memset(counter, 0, sizeof(uint32_t) * BinSize);

    for (std::size_t i = 0; i < n; ++i) {
      std::uint32_t index = a[i];
      counter[(index >> new_shift) & (BinSize - 1)]++;
    }

    std::size_t sum = 0;
    for (std::size_t i = 0; i < BinSize; ++i) {
      auto temp = counter[i];
      counter[i] = sum;
      sum += temp;
    }

    for (std::size_t i = 0; i < n; ++i) {
      std::uint32_t value = a[i];
      std::size_t offset = (value >> new_shift) & (BinSize - 1);
      buffer[counter[offset]++] = value;
    }
    std::swap(a, buffer);
  }
}

template <std::size_t BinSize = 256>
static void radix_sort_cache_thread_v1(std::uint32_t *a, std::size_t n) {
  RADIX_REQUIRE_ALLOWED_BINSIZE(BinSize);
  static constexpr std::size_t RSHIFT =
      details::helper::constexpr_log2(BinSize);
  static constexpr std::size_t ROUND =
      std::numeric_limits<uint32_t>::digits / RSHIFT;

  const int nproc = omp_get_max_threads();
  omp_set_num_threads(nproc);
  std::vector<uint32_t> bin(n);
  std::vector<uint32_t> counter(BinSize * nproc);
  uint32_t *buffer = bin.data();

  for (std::size_t round = 0; round < ROUND; ++round) {
    const std::size_t new_shift = RSHIFT * round;
    memset(counter.data(), 0, sizeof(uint32_t) * (BinSize * nproc));

#pragma omp parallel for
    for (long long i = 0; i < n; ++i) {
      std::uint32_t value = a[i];
      std::size_t offset = (value >> new_shift) & (BinSize - 1);
      counter[offset + BinSize * omp_get_thread_num()]++;
    }

    for (std::size_t i = 1; i < nproc; ++i) {
      for (std::size_t j = 0; j < BinSize; ++j) {
        counter[j] += counter[BinSize * i + j];
      }
    }

    std::size_t sum = 0;
    for (std::size_t i = 0; i < BinSize; ++i) {
      auto temp = counter[i];
      counter[i] = sum;
      sum += temp;
    }

    for (int i = 0; i < n; ++i) {
      std::uint32_t value = a[i];
      std::size_t offset = (value >> new_shift) & (BinSize - 1);
      buffer[counter[offset]++] = value;
    }
    std::swap(a, buffer);
  }
}

template <std::size_t BinSize = 256>
static void radix_sort_cache_thread_v2(std::uint32_t *a, std::size_t n) {
  RADIX_REQUIRE_ALLOWED_BINSIZE(BinSize);
  static constexpr std::size_t RSHIFT =
      details::helper::constexpr_log2(BinSize);
  static constexpr std::size_t ROUND =
      std::numeric_limits<uint32_t>::digits / RSHIFT;

  const int nproc = omp_get_max_threads();
  omp_set_num_threads(nproc);
  const long long chunk = (n + nproc - 1) / nproc;

  uint32_t base[BinSize]{};
  std::vector<uint32_t> bin(n);
  std::vector<uint32_t> local(BinSize * nproc);

  uint32_t *buffer = bin.data();

  for (std::size_t round = 0; round < ROUND; ++round) {
    const std::size_t new_shift = RSHIFT * round;
    memset(base, 0, sizeof(uint32_t) * BinSize);
    memset(local.data(), 0, sizeof(uint32_t) * (BinSize * nproc));

#pragma omp parallel for schedule(static, 1)
    for (long long thread_id = 0; thread_id < nproc; ++thread_id) {
      for (long long i = thread_id * chunk;
           i < (thread_id + 1) * chunk && i < n; ++i) {
        std::uint32_t value = a[i];
        std::size_t offset = (value >> new_shift) & (BinSize - 1);
        local[offset + BinSize * thread_id]++;
      }
    }

    for (std::size_t i = 0; i < nproc; ++i) {
      for (std::size_t j = 0; j < BinSize; ++j) {
        base[j] += local[BinSize * i + j];
      }
    }

    // for global!
    std::size_t sum = 0;
    for (std::size_t i = 0; i < BinSize; ++i) {
      auto temp = base[i];
      base[i] = sum;
      sum += temp;
    }

    // for local
    for (std::size_t j = 0; j < BinSize; ++j) {
      sum = 0;
      for (std::size_t i = 0; i < nproc; ++i) {
        auto temp = local[i * BinSize + j];
        local[i * BinSize + j] = sum;
        sum += temp;
      }
    }

#pragma omp parallel for schedule(static, 1)
    for (long long thread_id = 0; thread_id < nproc; ++thread_id) {
      for (long long i = thread_id * chunk;
           i < (thread_id + 1) * chunk && i < n; ++i) {
        std::uint32_t value = a[i];
        std::size_t offset = (value >> new_shift) & (BinSize - 1);
        buffer[base[offset] + local[thread_id * BinSize + offset]++] = value;
      }
    }

    std::swap(a, buffer);
  }
}
} // namespace details

template <
    std::size_t BinSize = 256,
    typename std::enable_if<(BinSize == 16u || BinSize == 256u ||
                             BinSize == 65536u || BinSize == 4294967296ull),
                            int>::type = 0>
static void radix_sort(std::vector<std::uint32_t> &vec) {
  details::radix_sort_cache_thread_v2<BinSize>(vec.data(), vec.size());
}
} // namespace radix
} // namespace sort

#endif // ! _RADIX_SORT_HPP_
