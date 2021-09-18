#pragma once

#include "os/str.hpp"
#include <stdexcept>
#include <string>
#include <unordered_map>

namespace conf {

namespace impl {
inline std::unordered_map<std::string, std::string> conf; // NOLINT static global
} // namespace impl

void init(const std::string& filename = "");

template <typename ValueType = std::string>
ValueType get(const std::string& key) {

  auto it = impl::conf.find(key); // using .contains() would mean 2 hash lookups
  if (it == impl::conf.end()) throw std::logic_error("no config value found for `" + key + "`\n");
  
  if constexpr (std::is_same_v<ValueType, std::string>)
    return it->second;
  else
    return os::str::parse<ValueType>(it->second.data());
}

} // namespace conf
