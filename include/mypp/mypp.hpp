#pragma once

#include "mysql.h"
#include "os/str.hpp"
#include <cstddef>
#include <cstring>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace mypp {

class result {
public:
  explicit result(MYSQL_RES* result) : myr(result) {}

  result(const result& m) = delete;
  result& operator=(const result& other) = delete;

  result(result&& other) noexcept = delete;
  result& operator=(result&& other) noexcept = delete;

  ~result() { ::mysql_free_result(myr); }

  unsigned num_fields() { return ::mysql_num_fields(myr); }

  std::vector<std::string> fieldnames() {
    std::vector<std::string> fieldnames;
    fieldnames.reserve(num_fields());
    for (unsigned i = 0; i < num_fields(); ++i) {
      MYSQL_FIELD* field = ::mysql_fetch_field(myr);
      fieldnames.emplace_back(field->name);
    }
    return fieldnames;
  }

  MYSQL_ROW fetch_row() { return ::mysql_fetch_row(myr); }

  struct Iterator {
    using iterator_category = std::input_iterator_tag;
    using difference_type   = std::ptrdiff_t;
    using value_type        = const MYSQL_ROW;
    using pointer           = const MYSQL_ROW*;
    using reference         = const MYSQL_ROW&;

    explicit Iterator(result& resultset, value_type row) : result_(resultset), currow_(row) {}

    reference operator*() const { return currow_; }
    pointer   operator->() const { return &currow_; }
    Iterator& operator++() {
      currow_ = result_.fetch_row();
      return *this;
    }
    Iterator operator++(int) { // NOLINT const weirdness
      Iterator tmp = *this;
      ++(*this);
      return tmp;
    }
    bool operator==(const Iterator& rhs) const { return currow_ == rhs.currow_; }
    bool operator!=(const Iterator& b) const { return currow_ != b.currow_; }

  private:
    result&   result_;
    MYSQL_ROW currow_; // atypically we store the value_type, because operator++ is a function call
  };

  Iterator begin() { return Iterator(*this, fetch_row()); }
  Iterator end() { return Iterator(*this, nullptr); }

private:
  MYSQL_RES* myr = nullptr;
};

class mysql {
public:
  mysql() {
    myp = ::mysql_init(myp);
    if (myp == nullptr) throw std::logic_error("mysql_init failed");
  }

  mysql(const mysql& m) = delete;
  mysql& operator=(const mysql& other) = delete;

  mysql(mysql&& other) noexcept = default; // needed to init static in con
  mysql& operator=(mysql&& other) noexcept = delete;

  ~mysql() { ::mysql_close(myp); }

  void connect(const std::string& host, const std::string& user, const std::string& password,
               const std::string& db, unsigned port = 0, const std::string& socket = "",
               std::uint64_t flags = 0UL) {

    if (::mysql_real_connect(myp, host.c_str(), user.c_str(), password.c_str(), db.c_str(), port,
                             socket.c_str(), flags) == nullptr) {
      throw std::logic_error("mysql connect failed");
    }
  }

  result query(const std::string& sql) {
    if (::mysql_query(myp, sql.c_str()) != 0) {
      throw std::logic_error("mysql query failed: " + std::string(::mysql_error(myp)));
    }
    MYSQL_RES* res = ::mysql_use_result(myp); // don't buffer. it's slightly slower
    if (res == nullptr) {
      throw std::logic_error("couldn't get results set: " + std::string(::mysql_error(myp)));
    }
    return result(res);
  }

  // throws if row not found
  std::vector<std::string> single_row(const std::string& sql) {
    auto      rs  = query(sql);
    MYSQL_ROW row = rs.fetch_row();
    if (row == nullptr) throw std::logic_error("single row not found by: " + sql);
    // must take copy in vector to avoid lifetime issues
    return std::vector<std::string>(row, row + rs.num_fields());
  }

  std::string single_value(const std::string& sql, unsigned col = 0) {
    auto      rs  = query(sql);
    MYSQL_ROW row = rs.fetch_row();
    if (row == nullptr) throw std::logic_error("single row not found by: " + sql);
    if (rs.num_fields() < col + 1)
      throw std::logic_error("column " + std::to_string(col) + " not found");
    return row[col]; // take copy as std::string
  }

  std::int64_t get_max_allowed_packet() {
    static std::int64_t max_allowed_packet =
        std::stol(single_value("show variables like 'max_allowed_packet'", 1));
    return max_allowed_packet;
  }

  std::string quote(const char* in) {
    // expensive to strlen and then allocate a std::string, but mysql_* api forces us?
    std::size_t len = strlen(in);
    std::string out(len * 2 + 2, '\0');
    out[0] = '\''; // place leading quote
    std::size_t newlen =
        ::mysql_real_escape_string(myp, &out[1], in, len); // leave leading quote in place

    if (newlen == static_cast<std::size_t>(-1)) {
      throw std::logic_error("mysql_real_escape_string failed: " + std::string(::mysql_error(myp)));
    }
    out[newlen + 1] = '\''; // fixup closing quote
    out.erase(newlen + 2);  // trim: checked that new null terminator is placed (libstdc++10)
    return out;
  }

private:
  MYSQL* myp = nullptr;
};

inline std::string quote_identifier(const std::string& identifier) {
  std::ostringstream os;
  os << '`' << os::str::replace_all_copy(identifier, "`", "``") << '`';
  return os.str();
}

} // namespace mypp
