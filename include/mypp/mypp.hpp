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

class mysql;
class result;
class row;

mypp::mysql& con();

// representing a mysql/mariadb connection handle. Wrapper for MYSQL*
class mysql {
public:
  mysql();

  mysql(const mysql& m) = delete;
  mysql& operator=(const mysql& other) = delete;

  mysql(mysql&& other) noexcept = default; // needed to init static in con
  mysql& operator=(mysql&& other) noexcept = delete;

  ~mysql() { ::mysql_close(mysql_); }

  void connect(const std::string& host, const std::string& user, const std::string& password,
               const std::string& db, unsigned port = 0, const std::string& socket = "",
               std::uint64_t flags = 0UL);

  result                   query(const std::string& sql, bool expect_result = true);
  std::vector<std::string> single_row(const std::string& sql); // throws if row not found
  std::string              single_value(const std::string& sql, unsigned col = 0);
  std::int64_t             get_max_allowed_packet();
  std::string              quote(const char* in);

  unsigned    errnumber() { return ::mysql_errno(mysql_); }
  std::string error() { return ::mysql_error(mysql_); }

private:
  MYSQL* mysql_ = nullptr;
};

// the result set obtained from a query. Wrapper for MYSQL_RES.
class result {
public:
  result(mysql* mysql_, MYSQL_RES* result) : mysql(mysql_), myr(result) {}

  result(const result& m) = delete;
  result& operator=(const result& other) = delete;

  result(result&& other) noexcept = delete;
  result& operator=(result&& other) noexcept = delete;

  ~result() { ::mysql_free_result(myr); }

  unsigned                 num_fields() { return ::mysql_num_fields(myr); }
  std::vector<std::string> fieldnames();
  row                      fetch_row();

  struct Iterator;
  Iterator begin();
  Iterator end();

  mysql* mysql;

private:
  MYSQL_RES* myr = nullptr;
};

// representing one row of a resultset. Wrapper for MYSQL_ROW
class row {
public:
  row(result& result_set, MYSQL_ROW row) : rs(&result_set), row_(row) {}

  result* rs;

  bool empty() { return row_ == nullptr; }

  [[nodiscard]] std::vector<std::string> vector() const {
    return std::vector<std::string>(row_, row_ + rs->num_fields());
  }

  [[nodiscard]] char* operator[](unsigned idx) const { return row_[idx]; }

  [[nodiscard]] char* at(unsigned idx) const {
    if (idx >= rs->num_fields()) throw std::logic_error("field idx out of bounds");
    return row_[idx];
  }

  bool operator==(const row& rhs) const { return row_ == rhs.row_; }
  bool operator!=(const row& rhs) const { return row_ != rhs.row_; }

private:
  MYSQL_ROW row_;
};

std::string quote_identifier(const std::string& identifier);

// Iterates over a result set
struct result::Iterator {
  using iterator_category = std::input_iterator_tag;
  using difference_type   = std::ptrdiff_t;
  using value_type        = const row;
  using pointer           = const row*;
  using reference         = const row&;

  explicit Iterator(value_type row) : currow_(row) {}

  reference operator*() const { return currow_; }
  pointer   operator->() const { return &currow_; }
  Iterator& operator++() {
    currow_ = currow_.rs->fetch_row();
    return *this;
  }
  Iterator operator++(int) { // NOLINT const weirdness
    Iterator tmp = *this;
    ++(*this);
    return tmp;
  }
  bool operator==(const Iterator& rhs) const { return currow_ == rhs.currow_; }
  bool operator!=(const Iterator& rhs) const { return currow_ != rhs.currow_; }

private:
  row currow_; // atypically we store the value_type, because operator++ is a function
};

} // namespace mypp
