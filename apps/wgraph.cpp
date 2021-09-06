#include <algorithm>
#include <iomanip>
#include <ios>
#include <iostream>
#include <numeric>
#include <string>
#include <vector>
#include <deque>

constexpr std::size_t window_ahead  = 3;
constexpr std::size_t window_behind = 5;

void print_window(const std::deque<std::string>& window) {
  std::size_t count = 0;
  for (auto&& s: window) {
    ++count;
    if (count == window_behind + 1 || count == window_behind + 2) {
      std::cout << "    ";
    }
    std::cout << s << ", ";
  }
  std::cout << "\n";
}

// class window {
//  public:
//  private:
//   map_
//   }

int main() {
  std::ios::sync_with_stdio(false);
  std::string                                  w;
  std::unordered_map<std::string, std::size_t> map;
  using map_entry_t = std::pair<decltype(map)::key_type, decltype(map)::mapped_type>;

  auto is_not_alpha = [](unsigned char c) { return std::isalpha(c) == 0; };
  auto chrtolower   = [](unsigned char c) { return std::tolower(c); };

  std::size_t             count = 0;
  std::deque<std::string> window;

  while (std::cin >> w) {
    w.erase(std::remove_if(begin(w), end(w), is_not_alpha), end(w));
    std::transform(begin(w), end(w), begin(w), chrtolower);
    if (!w.empty()) {
      if (window.size() < window_ahead + window_behind + 1) {
        print_window(window);
        // pre fill stage
        for (std::size_t i = 0; i < window.size(); ++i) {
          if (i != window.size() -1) { // ie not "current"
            std::cout << window[window.size() - 1] << " " << window[i] << "\n";
          }
        }
      } else {
        print_window(window);
        for (std::size_t i = 0; i != window.size(); ++i) {
          if (i != window_behind) { // ie not "current"
            std::cout << window[window_behind] << " " << window[i] << "\n";
          }
        }
      }
      ++count;
      ++map[w];
      window.push_back(w);
      if (window.size() < window_ahead + window_behind + 2) continue;
      window.pop_front();
    }
  }
  while (window.size() > window_behind) {
    print_window(window);
    for (std::size_t i = 0; i != window.size(); ++i) {
      if (i != window_behind) { // ie not "window_behind"
        std::cout << window[window_behind] << " " << window[i] << "\n";
      }
    }
    window.pop_front();
  }
  constexpr std::size_t    N = 10;
  std::vector<map_entry_t> topN(N);
  std::partial_sort_copy(begin(map), end(map), begin(topN), end(topN),
                         [](auto& a, auto& b) { return a.second > b.second; });

  std::size_t wc = std::accumulate(begin(map), end(map), 0,
                                   [](int tot, const auto& p) { return tot + p.second; });

  // output
  std::cout.imbue(std::locale(""));
  std::cout << std::setw(12) << std::left << "word count" << std::setw(10) << std::right << wc
            << "\n";
  std::cout << std::setw(12) << std::left << "unique words" << std::setw(10) << std::right
            << map.size() << "\n\n";
  std::cout << "Top " << N << " words\n\n";
  for (auto&& [word, freq]: topN)
    std::cout << std::setw(10) << std::left << word << std::right << std::setw(8) << freq
              << std::setw(6) << std::fixed << std::setprecision(2) << 100.0 * freq / wc << "%\n";

  exit(EXIT_SUCCESS);
}
