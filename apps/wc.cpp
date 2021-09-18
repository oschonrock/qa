#include "conf/conf.hpp"
#include "mypp/mypp.hpp"
#include "os/bch.hpp"
#include <string>
#include <vector>

mypp::mysql& con() {
  static auto con = []() {
    using conf::get;
    mypp::mysql my;

    // clang-format off
    my.connect(conf::get("db_host"),
               conf::get("db_user"),
               conf::get("db_pass"),
               conf::get("db_db"),
               conf::get<unsigned>("db_port"),
               conf::get("db_socket"));
    // clang-format on
    return my;
  }();
  return con;
}

struct member {
  std::string firstname;
  std::string lastname;
  std::string email;

  friend std::ostream& operator<<(std::ostream& os, const member& m) {
    os << m.firstname << " " << m.lastname << " <" << m.email << ">";
    return os;
  }
};

int main() {
  conf::init("./build/bin/myslice_demo.ini");

  std::vector<member> members;
  for (auto&& r: con().query("select firstname, lastname, email from member"))
    members.push_back({r.get(0), r.get(1), r.get(2)});

  for (auto&& m: members) std::cout << m << "\n";
}
