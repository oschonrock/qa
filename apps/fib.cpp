#include <array>
#include <bits/c++config.h>
#include <iostream>
#include <utility>

template <int I>
struct Fib {
  static const int val = Fib<I - 1>::val + Fib<I - 2>::val;
};

template <>
struct Fib<0> {
  static const int val = 0;
};

template <>
struct Fib<1> {
  static const int val = 1;
};

template<std::size_t i>
constexpr int fib() {
  if constexpr (i == 0 || i == 1) 
    return i;
  else 
    return fib<i-1>() + fib<i-2>();
}

int main() {
  std::cout << Fib<45>::val << "\n";
  std::cout << fib<45>() << "\n";
}
