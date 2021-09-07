#include <iostream>
#include <numeric>
#include <string>
#include <vector>

inline bool is_pythag_multiple(int a, int b, int c) noexcept {
  return std::gcd(std::gcd(a, b), std::gcd(a, c)) > 1;
}

inline bool is_pythag_triple(int a, int b, int c) noexcept { return a * a + b * b == c * c; }

template <typename F>
inline void find_pythag_triples(F&& process_triple) {
  for (int c = 1;; ++c)
    for (int a = 1; a != c; ++a)
      for (int b = a; b != c; ++b)
        if (is_pythag_triple(a, b, c) && !is_pythag_multiple(a, b, c) && !process_triple(a, b, c))
          return;
}

int main(int argc, char* argv[]) {
  std::vector<std::string> args(argv, argv + argc);

  if (args.size() < 2) {
    std::cerr << "Usage: " << args[0] << " TopN\n";
    return EXIT_FAILURE;
  }
  int n = std::stoi(args[1]);

  if (n < 1) {
    std::cerr << "N should be 1 or greater\n";
  }

  std::cout << "Finding top " << n << " Pythagorean triples\n";

  find_pythag_triples([&n](int a, int b, int c) {
    std::cout << a << "² + " << b << "² = " << c << "²" << '\n';
    return --n;
  });
}
