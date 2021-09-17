#include "mypp.hpp"

namespace mypp {

std::string quote_identifier(const std::string& identifier) {
  std::ostringstream os;
  os << '`' << os::str::replace_all_copy(identifier, "`", "``") << '`';
  return os.str();
}

// mypp::mysql

mysql::mysql() {
  // in a multithreaded environment mysql_library_init should be protected by a mutex
  if (mysql_library_init(0, nullptr, nullptr) != 0)
    throw std::logic_error("mysql_library_init failed");
  mysql_ = ::mysql_init(mysql_);
  if (mysql_ == nullptr) throw std::logic_error("mysql_init failed");
}

void mysql::connect(const std::string& host, const std::string& user, const std::string& password,
                    const std::string& db, unsigned port, const std::string& socket,
                    std::uint64_t flags) {

  if (::mysql_real_connect(mysql_, host.c_str(), user.c_str(), password.c_str(), db.c_str(), port,
                           socket.c_str(), flags) == nullptr) {
    throw std::logic_error("mysql connect failed");
  }
}


result mysql::query(const std::string& sql, bool expect_result) {
  if (::mysql_query(mysql_, sql.c_str()) != 0) {
    throw std::logic_error("mysql_query failed: " + error());
  }
  MYSQL_RES* res = ::mysql_use_result(mysql_); // don't buffer. it's slightly slower
  if (expect_result && res == nullptr) {
    throw std::logic_error("couldn't get results set for query: " + sql + "  Error was:" + error());
  }
  return result(this, res);
}

// throws if row not found
std::vector<std::string> mysql::single_row(const std::string& sql) {
  auto rs  = query(sql);
  auto row = rs.fetch_row();
  if (row.empty()) throw std::logic_error("single row not found by: " + sql);
  // must take copy in vector to avoid lifetime issues
  return row.vector();
}

int mysql::get_max_allowed_packet() {
  static int max_allowed_packet =
      single_value<int>("show variables like 'max_allowed_packet'", 1);
  return max_allowed_packet;
}

std::string mysql::quote(const char* in) {
  // expensive to strlen and then allocate a std::string, but mysql_* api forces us?
  std::size_t len = strlen(in);
  std::string out(len * 2 + 2, '\0');
  out[0] = '\''; // place leading quote
  std::size_t newlen =
      ::mysql_real_escape_string(mysql_, &out[1], in, len); // leave leading quote in place

  if (newlen == static_cast<std::size_t>(-1)) {
    throw std::logic_error("mysql_real_escape_string failed: " + error());
  }
  out[newlen + 1] = '\''; // fixup closing quote
  out.erase(newlen + 2);  // trim: checked that new null terminator is placed (libstdc++10)
  return out;
}

// mypp::result

row result::fetch_row() {
  auto* r = ::mysql_fetch_row(myr);
  // https://dev.mysql.com/doc/c-api/5.7/en/mysql-fetch-row.html
  if (r == nullptr && mysql->errnumber() != 0)
    throw std::logic_error("mysql_fetch_row failed:" + mysql->error());
  return row(*this, r);
}

result::Iterator result::begin() { return Iterator(fetch_row()); }
result::Iterator result::end() { return Iterator(row(*this, nullptr)); }

std::vector<std::string> result::fieldnames() {
  std::vector<std::string> fieldnames;
  fieldnames.reserve(num_fields());
  for (unsigned i = 0; i < num_fields(); ++i) {
    MYSQL_FIELD* field = ::mysql_fetch_field(myr);
    fieldnames.emplace_back(field->name);
  }
  return fieldnames;
}

} // namespace mypp
