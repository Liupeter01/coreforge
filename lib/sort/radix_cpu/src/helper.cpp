#include <radix_sort_cpu.hpp>
#include <random>
#include <algorithm>

void sort::radix::details::helper::write_back(
    std::uint32_t *a, const std::vector<std::vector<uint32_t>> &bin,
    std::size_t n) {
  std::size_t index = 0;
  for (std::size_t i = 0; i < n; ++i) {
    auto bin_size = bin[i].size();
    for (std::size_t elem = 0; elem < bin_size; ++elem)
      a[index++] = bin[i][elem];
  }
}

void sort::radix::details::helper::clean_bin(
    std::vector<std::vector<uint32_t>> &bins) {
  for (auto &bin : bins)
    bin.clear();
}

void sort::radix::details::helper::generate_random(std::vector<uint32_t> &vec,
                                                   const std::size_t numbers) {
  vec.clear();
  vec.resize(numbers);
  std::generate(vec.begin(), vec.end(),
                [uni = std::uniform_int_distribution<uint32_t>(0, UINT32_MAX),
                 rng = std::mt19937{}]() mutable { return uni(rng); });
}
