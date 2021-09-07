#include "fmt/core.h"
#include <algorithm>
#include <cstdint>
#include <iomanip>
#include <iostream>
#include <numeric>
#include <string>
#include <unordered_map>
#include <vector>

using std::int32_t;

struct split_on_non_alpha : std::ctype<char> {
  static const mask* make_table() {
    static std::vector<mask> v(classic_table(), classic_table() + table_size); // NOLINT ptr arith
    for (unsigned c = 0; c != table_size; ++c)
      if ((v[c] & alpha) == 0) v[c] = space; // all non-alpha are space
    return v.data();
  }
  explicit split_on_non_alpha(std::size_t refs = 0) : ctype(make_table(), false, refs) {}
};

int main() {
  std::unordered_map<std::string, std::size_t> map;
  using map_entry_t = std::pair<decltype(map)::key_type, decltype(map)::mapped_type>;

  std::ios::sync_with_stdio(false); //  speed
  std::cin.imbue(std::locale(std::cin.getloc(), new split_on_non_alpha));

  std::string w;
  while (std::cin >> w) {
    std::transform(begin(w), end(w), begin(w), [](unsigned char c) { return std::tolower(c); });
    ++map[w]; // can't be empty, due to split algo above, so no need to check
  }

  constexpr std::size_t    N = 10;
  std::vector<map_entry_t> topN(std::min(N, map.size()));
  std::partial_sort_copy(begin(map), end(map), begin(topN), end(topN),
                         [](auto& a, auto& b) { return a.second > b.second; });

  std::size_t wc = std::accumulate(begin(map), end(map), 0UL,
                                   [](std::size_t tot, const auto& p) { return tot + p.second; });

  std::locale::global(std::locale("")); // user preferred locale
  fmt::print("{:<12s} {:>10Ld}\n", "word count", wc);
  fmt::print("{:<12s} {:>10Ld}\n\n", "unique words", map.size());
  fmt::print("Top {:d} words\n\n", N);
  for (auto&& [word, freq]: topN)
    fmt::print("{:<10s} {:>8Ld} {:>6.2f}%\n", word, freq, 100.0L * freq / wc);
}
