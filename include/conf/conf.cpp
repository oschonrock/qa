#include "conf/conf.hpp"
#include "os/str.hpp"
#include <fstream>
#include <stdexcept>
#include <string>
#include <unordered_map>

namespace conf {

void init(const std::string& filename) {
  if (filename.length() == 0)
    throw std::logic_error("must provide config filename to `conf_init()`");

  std::ifstream configfile(filename);
  if (!configfile.is_open())
    throw std::logic_error("could not open config file: tried `" + filename + "`.");

  for (std::string line; std::getline(configfile, line);) {
    os::str::trim(line);
    if (line.empty()) continue;
    auto pos = line.find('=');
    if (pos == std::string::npos)
      throw std::logic_error("illegal configfile syntax. use `name=value` on each line");
    auto key   = line.substr(0, pos);
    auto value = line.substr(pos + 1);

    os::str::trim(key);
    if (key.length() == 0)
      throw std::logic_error("config file key is empty on line: `" + line + "`");
    if (impl::conf.contains(key))
      throw std::logic_error("duplicate key error in config file: `" + key + "`");

    os::str::trim(value);
    if (value.length() == 0)
      throw std::logic_error("config file value is empty on line: `" + line + "`");

    impl::conf.emplace(std::move(key), std::move(value));
  }
}

} // namespace conf
