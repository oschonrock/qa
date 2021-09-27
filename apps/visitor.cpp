#include <algorithm>
#include <benchmark/benchmark.h>
#include <cstddef>
#include <random>
#include <unordered_set>
#include <vector>

void gen1(benchmark::State& state) {
  auto                                       no_nodes = static_cast<std::size_t>(state.range(0));
  std::size_t                                no_edges = no_nodes * 2;
  std::mt19937_64                            rgen(1); // NOLINT fixed seed
  std::uniform_int_distribution<std::size_t> dist(0, no_nodes - 1);

  std::vector<std::size_t> rvs(no_edges);
  std::generate(begin(rvs), end(rvs), [&dist, &rgen]() { return dist(rgen); });

  for (auto _: state) {
    std::vector<std::size_t> uniqverts;
    std::copy(rvs.begin(), rvs.end(), uniqverts.begin());
    std::sort(begin(uniqverts), end(uniqverts));
    uniqverts.erase(std::unique(begin(uniqverts), end(uniqverts)), uniqverts.end());
    std::size_t size = uniqverts.size();
    std::size_t min  = uniqverts.front();
    std::size_t max  = uniqverts.back();
    benchmark::DoNotOptimize(size);
    benchmark::DoNotOptimize(min);
    benchmark::DoNotOptimize(max);
  }
}
BENCHMARK(gen1)->Range(8, 8U << 16U);

void gen2(benchmark::State& state) {
  auto                                       no_nodes = static_cast<std::size_t>(state.range(0));
  std::size_t                                no_edges = no_nodes * 2;
  std::mt19937_64                            rgen(1); // NOLINT fixed seed
  std::uniform_int_distribution<std::size_t> dist(0, no_nodes - 1);

  std::vector<std::size_t> rvs(no_edges);
  std::generate(begin(rvs), end(rvs), [&dist, &rgen]() { return dist(rgen); });

  for (auto _: state) {
    std::unordered_set<std::size_t> uniqset;
    for (auto&& e: rvs) uniqset.insert(e);
    auto [min_it, max_it] = std::minmax_element(begin(uniqset), end(uniqset));
    std::size_t size      = uniqset.size();
    std::size_t min       = *min_it;
    std::size_t max       = *max_it;
    benchmark::DoNotOptimize(size);
    benchmark::DoNotOptimize(min);
    benchmark::DoNotOptimize(max);
  }
}
BENCHMARK(gen2)->Range(8, 8U << 16U);

BENCHMARK_MAIN();
