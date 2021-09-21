#include "conf/conf.hpp"
#include "date/date.h"
#include "mypp/mypp.hpp"
#include "os/bch.hpp"
#include <ctime>
#include <filesystem>
#include <iomanip>
#include <optional>
#include <sstream>
#include <string>
#include <vector>

mypp::mysql& db() {
  static auto db = []() {
    using conf::get;
    mypp::mysql db;
    db.connect(conf::get_or("db_host", "localhost"), conf::get("db_user"), conf::get("db_pass"),
               conf::get("db_db"), conf::get_or<unsigned>("db_port", 0U),
               conf::get_or("db_socket", ""));

    constexpr auto charset = "utf8";
    db.set_character_set(charset);

    std::cerr << "Notice: Connected to " << conf::get("db_db") << " on " << db.get_host_info()
              << " using " << charset << "\n";
    return db;
  }();
  return db;
}

struct member {
  std::string                      firstname;
  std::string                      lastname;
  std::string                      email;
  std::optional<date::sys_days>    dob;
  std::optional<date::sys_seconds> date_of_last_logon;
  std::optional<date::sys_seconds> created_at;
  std::optional<date::sys_seconds> updated_at;
  int                              email_failure_count;
  bool                             invalid;
  std::optional<date::sys_seconds> invalidated_time;

  friend std::ostream& operator<<(std::ostream& os, const member& m) {
    using mypp::format_time_point;

    os << m.firstname << " " << m.lastname << " <" << m.email << ">\n";
    if (m.dob) os << "dob=" << format_time_point(m.dob.value()) << "\n";
    if (m.date_of_last_logon)
      os << "date_of_last_logon=" << format_time_point(m.date_of_last_logon.value()) << "\n";
    if (m.created_at) os << "created_at=" << format_time_point(m.created_at.value()) << "\n";
    if (m.updated_at) os << "updated_at=" << format_time_point(m.updated_at.value()) << "\n";
    os << "email_failure_count=" << m.email_failure_count << "\n";
    os << "invalid=" << std::boolalpha << m.invalid << "\n"
       << "invalidated_time=";
    if (m.invalidated_time) os << format_time_point(m.invalidated_time.value());
    os << "\n";
    return os;
  }
};

int main(int argc, char* argv[]) {
  std::vector<std::string> args(argv, argv + argc);
  std::ios::sync_with_stdio(false);

  try {
    conf::init(std::filesystem::path(args[0]).parent_path().append("myslice_demo.ini"));

    std::vector<member> members;
    for (auto&& r: db().query("select "
                              "  firstname, "
                              "  lastname, "
                              "  email, "
                              "  dob, "
                              "  date_of_last_logon, "
                              "  created_at, "
                              "  updated_at, "
                              "  email_failure_count, "
                              "  invalid, "
                              "  invalidated_time "
                              "from member")) {

      members.push_back(member{
        // clang-format off
        .firstname              = r.get(0),
        .lastname               = r.get(1),
        .email                  = r.get(2),
        .dob                    = r.get<std::optional<date::sys_days>>(3),
        .date_of_last_logon     = r.get<std::optional<date::sys_seconds>>(4),
        .created_at             = r.get<std::optional<date::sys_seconds>>(5),
        .updated_at             = r.get<std::optional<date::sys_seconds>>(6),
        .email_failure_count    = r.get<int>(7),
        .invalid                = r.get<bool>(8),
        .invalidated_time       = r.get<std::optional<date::sys_seconds>>(9)
        // clang-format on
      });
    }

    for (auto&& m: members) std::cout << m << "\n";

  } catch (const std::exception& e) {
    std::cerr << "Something went wrong. Exception thrown: " << e.what() << std::endl;
  }
}
