#include "os/bch.hpp"
#include <iostream>
#include <iterator>
#include <random>
#include <unordered_set>

std::unordered_set<int> intersect(const std::unordered_set<int>& s1,
                                  const std::unordered_set<int>& s2) {

  auto [l, r] = [&]() {
    if (s1.size() < s2.size()) return std::tie(s1, s2);
    return std::tie(s2, s1);
  }();

  std::unordered_set<int> result;
  result.reserve(l.size());

  for (const auto& v: l) 
    if (r.contains(v)) result.insert(v);
  
  return result;
}

int main() {
  for (int n = 10; n < 10'000'000; n *= 10) {
    std::cout << "======= N = " << n << " ==============\n";
    {
      std::random_device              rd;
      std::mt19937                    gen(1);
      std::uniform_int_distribution<> distrib(1, 10 * n);

      std::unordered_set<int> set1;
      std::unordered_set<int> set2;
      std::unordered_set<int> set3;

      {
        os::bch::Timer t1("fill set1");
        for (int i = 0; i < n; ++i) set1.insert(distrib(gen));
      }

      {
        os::bch::Timer t1("fill set2");
        for (int i = 0; i < n / 2; ++i) set2.insert(distrib(gen));
      }

      {
        os::bch::Timer t1("intersect");
        set3 = intersect(set1, set2);
      }

      std::cout << set3.size() << "\n";
    }

    {
      std::random_device              rd;
      std::mt19937                    gen(1);
      std::uniform_int_distribution<> distrib(1, 10 * n);

      std::vector<int> set1;
      std::vector<int> set2;
      std::vector<int> set3;

      {
        os::bch::Timer t1("fill set1");
        for (int i = 0; i < n; ++i) set1.emplace_back(distrib(gen));
      }

      {
        os::bch::Timer t1("fill set2");
        for (int i = 0; i < n / 2; ++i) set2.emplace_back(distrib(gen));
      }

      {
        os::bch::Timer t1("intersect");
        {
          // os::bch::Timer t1a("sort set1");
          std::sort(set1.begin(), set1.end());
        }
        {
          // os::bch::Timer t1a("sort set1");
          std::sort(set2.begin(), set2.end());
        }

        {
          // os::bch::Timer t1c("set_intersection");
          set3.reserve(std::min(set1.size(), set2.size()));
          std::set_intersection(set1.begin(), set1.end(), set2.begin(), set2.end(),
                                std::back_inserter(set3));
        }
      }

      std::cout << set3.size() << "\n";
    }
  }
}
