#include <iomanip>
#include <iostream>
#include <type_traits>

const long double PI   = 3.141592653589793238462643383279502884L;
const long double PI_2 = 1.570796326794896619231321691639751442L;

template <typename T, std::enable_if_t<std::is_floating_point_v<T>, bool> = true>
constexpr T arctan_taylor(T tan) {
  if (std::fabs(tan) > 1) return std::copysign(T(PI_2), tan) - arctan_taylor(1 / tan);
  T num  = tan;
  T den  = 1;
  T sign = 1;
  T term = num;
  T res  = num;
  while (res + std::fabs(term) != res) {
    num *= tan * tan;
    den += 2; // will remain precise for T = float up to 2^(mantissa bits + 1) + 1 = 16,777,217
    sign = -sign;
    term = sign * num / den;
    res += term;
  }
  return res;
}

template <typename T, std::enable_if_t<std::is_floating_point_v<T>, bool> = true>
constexpr T arctan(T tan) {
  if (std::fabs(tan) > 1) return std::copysign(T(PI_2), tan) - arctan(1 / tan);
  T sum  = 0;
  T prod = 1;
  for (int n = 0; sum + std::fabs(prod) != sum; ++n) {
    prod = 1;
    for (int k = 1; k <= n; ++k)
      prod *= 2 * k * tan * tan / ((2 * k + 1) * (1 + tan * tan)); //  Eulers Series
    sum += prod;
  }
  return tan / (1 + tan * tan) * sum;
}

template <typename T, std::enable_if_t<std::is_integral_v<T>, bool> = true>
constexpr double arctan(T tan) {
  return arctan(static_cast<double>(tan));
}

template <typename T, std::enable_if_t<std::is_floating_point_v<T>, bool> = true>
constexpr T pi() {
  return (4 * arctan(T(1) / 5) - arctan(T(1) / 239)) * 4;
}

int main() {
  auto pi_float       = pi<float>();
  auto pi_double      = pi<double>();
  auto pi_long_double = pi<long double>();

  std::cout << std::setprecision(std::numeric_limits<decltype(pi_float)>::digits10) << pi_float
            << '\n'
            << std::setprecision(std::numeric_limits<decltype(pi_double)>::digits10) << pi_double
            << '\n'
            << std::setprecision(std::numeric_limits<decltype(pi_long_double)>::digits10)
            << pi_long_double << '\n'
            << PI << '\n';

  std::cout << 1 / (arctan(-std::sqrt(3.0L)) / PI);
  return EXIT_SUCCESS;
}
