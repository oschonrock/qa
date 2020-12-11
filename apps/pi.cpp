#include <bits/stdint-intn.h>
#include <cmath>
#include <iomanip>
#include <iostream>
#include <cstdio>


template <typename T>
T arctan(T tan) {
  T num = tan;
  T den = 1.0;
  T term = num;
  T res = num;
  T sign = 1;
  while (std::fabs(term) > std::numeric_limits<T>::epsilon()) {
    num *= tan * tan;
    den += 2.0;
    sign *= -1.0;
    term = sign * num / den;
    res += term;
  }
  return res;
}

template <typename T>
T pi() {
  T one_over_5 = static_cast<T>(1.0) / static_cast<T>(5.0);
  T one_over_239 = static_cast<T>(1.0) / static_cast<T>(239.0);
  return (static_cast<T>(4.0) * arctan<T>(one_over_5) - arctan<T>(one_over_239)) * static_cast<T>(4.0);
}

int main() {
  auto pi_v = pi<long double>();
  std::cout << std::setprecision(std::numeric_limits<decltype(pi_v)>::digits10) << pi_v << '\n'
            << M_PI << '\n';

  return EXIT_SUCCESS;
}
