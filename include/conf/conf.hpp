#pragma once

#include "os/str.hpp"
#include <stdexcept>
#include <string>
#include <unordered_map>

namespace conf {

namespace impl {
inline std::unordered_map<std::string, std::string> conf; // NOLINT static global

template <typename ValueType>
ValueType maybe_parse(const std::string& value) {
  if constexpr (std::is_convertible_v<std::string, ValueType>)
    return value; // don't need to parse
  else if constexpr (std::is_same_v<ValueType, const char*>)
    return value.data(); // high speed return path
  else
    return os::str::parse<ValueType>(value.data());
}

} // namespace impl

void init(const std::string& filename = "");

template <typename ValueType = std::string>
ValueType get(const std::string& key) {

  auto it = impl::conf.find(key);
  if (it == impl::conf.end()) throw std::logic_error("no config value found for `" + key + "`\n");

  return impl::maybe_parse<ValueType>(it->second);
}

template <typename ValueType = std::string, typename DefaultType>
ValueType get_or(const std::string& key, DefaultType default_value) {
  static_assert(std::is_convertible_v<DefaultType, ValueType>,
                "must provide default of compatible type");

  auto it = impl::conf.find(key);
  if (it == impl::conf.end()) return default_value;

  return impl::maybe_parse<ValueType>(it->second);
}

} // namespace conf
