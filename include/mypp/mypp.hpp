#pragma once

#include "fmt/include/fmt/core.h"
#include "mysql.h"
#include "os/str.hpp"
#include <cstddef>
#include <cstring>
#include <sstream>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <unordered_set>
#include <vector>

namespace mypp {

class mysql;
class result;
class row;

mypp::mysql& con();

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

  template <typename ValueType>
  ValueType get(unsigned idx) const {
    return static_cast<void>((*this)[idx]); // should always fail? so we need a specialisation
  }

  template <>
  [[nodiscard]] int get<int>(unsigned idx) const {
    return os::str::parse_int((*this)[idx]); // faster than std::stoi, which makes a std::string tmp
  }

  template <>
  [[nodiscard]] std::string get<std::string>(unsigned idx) const {
    return (*this)[idx];
  }

  bool operator==(const row& rhs) const { return row_ == rhs.row_; }
  bool operator!=(const row& rhs) const { return row_ != rhs.row_; }

private:
  MYSQL_ROW row_;
};

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

template <typename C, typename V, typename = void>
struct has_push_back : std::false_type {};

template <typename C, typename V>
struct has_push_back<C, V, std::void_t<decltype(std::declval<C>().push_back(std::declval<V>()))>>
    : std::true_type {};

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

  template <typename ContainerType>
  ContainerType single_column(const std::string& sql, unsigned col = 0) {
    ContainerType values;
    auto          rs = query(sql);
    using ValueType  = typename ContainerType::value_type;
    for (auto&& row: rs) {
      if constexpr (has_push_back<ContainerType, ValueType>::value)
        values.push_back(row.get<ValueType>(col));
      else
        values.insert(row.get<ValueType>(col));
    }
    return values;
  }

  std::int64_t get_max_allowed_packet();
  std::string  quote(const char* in);

  unsigned    errnumber() { return ::mysql_errno(mysql_); }
  std::string error() { return ::mysql_error(mysql_); }

  void begin() { query("begin", false); }
  void rollback() { query("rollback", false); }

private:
  MYSQL* mysql_ = nullptr;
};

// representing one row of a resultset. Wrapper for MYSQL_ROW
std::string quote_identifier(const std::string& identifier);

} // namespace mypp
