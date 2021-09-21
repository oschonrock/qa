#pragma once

#include "date/date.h"
#include "fmt/core.h"
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

namespace impl {

template <typename... Args>
void whatis();

template <typename C, typename = void>
struct has_push_back : std::false_type {};

template <typename C>
struct has_push_back<
    C, std::void_t<decltype(std::declval<C>().push_back(std::declval<typename C::value_type>()))>>
    : std::true_type {};

template <typename T, typename = void>
struct is_optional : std::false_type {};

template <typename T>
struct is_optional<T, std::void_t<decltype(std::declval<T>().value())>> : std::true_type {};

template <typename DateType>
inline constexpr auto date_format = "bad format";
template <>
inline constexpr auto date_format<date::sys_days> = "%Y-%m-%d";
template <>
inline constexpr auto date_format<date::sys_seconds> = "%Y-%m-%d %H:%M:%S";

// mysql/mariadb specific parsing
template <typename NumericType>
inline NumericType parse(const char* str) {
  if constexpr (is_optional<NumericType>::value) {
    if (str == nullptr) return std::nullopt;
    using InnerType = std::remove_reference_t<decltype(std::declval<NumericType>().value())>;
    return parse<InnerType>(str);

  } else {
    if (str == nullptr)
      throw std::domain_error("requested type was not std::optional, but db returned NULL");

    if constexpr (std::is_same_v<NumericType, bool>) {
      return parse<long>(str) != 0; // db uses int{0} and int{1} for bool

    } else if constexpr (std::is_same_v<NumericType, date::sys_days> ||
                         std::is_same_v<NumericType, date::sys_seconds>) {
      std::istringstream stream(str);
      NumericType        t;
      date::from_stream(stream, date_format<NumericType>, t);
      if (stream.fail())
        throw std::domain_error("failed to parse date: " + std::string(str) + " with format " +
                                date_format<NumericType>);
      return t;
    } else {
      // delegate to general numeric formats
      return os::str::parse<NumericType>(str);
    }
  }
}

} // namespace impl

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

  template <typename ValueType = std::string>
  [[nodiscard]] ValueType get(unsigned idx) const {
    // try numeric parsing by default
    return impl::parse<ValueType>((*this)[idx]);
  }

  // takes a copy in a std::string
  template <>
  [[nodiscard]] std::string get<std::string>(unsigned idx) const {
    return (*this)[idx];
  }

  // access to the raw const char*
  template <>
  [[nodiscard]] const char* get<const char*>(unsigned idx) const {
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

  void set_character_set(const std::string& charset);

  std::string get_host_info();

  result query(const std::string& sql, bool expect_result = true);

  std::vector<std::string> single_row(const std::string& sql);

  // throws if row not found
  template <typename ValueType>
  ValueType single_value(const std::string& sql, unsigned col = 0) {
    static_assert(!std::is_same_v<ValueType, const char*>,
                  "single_value<const char*> will result in dangling pointers");
    auto rs  = query(sql);
    auto row = rs.fetch_row();
    if (row.empty()) throw std::logic_error("single row not found by: " + sql);
    if (rs.num_fields() < col + 1)
      throw std::logic_error("column " + std::to_string(col) + " not found");
    return row.get<ValueType>(col); // take copy in appropriate type
  }

  template <typename ContainerType>
  ContainerType single_column(const std::string& sql, unsigned col = 0) {
    ContainerType values;
    auto          rs = query(sql);
    using ValueType  = typename ContainerType::value_type;
    static_assert(!std::is_same_v<ValueType, const char*>,
                  "single_column<Container<const char*>> will result in dangling pointers");
    for (auto&& row: rs) {
      if constexpr (impl::has_push_back<ContainerType>::value)
        values.push_back(row.get<ValueType>(col));
      else
        values.insert(row.get<ValueType>(col));
    }
    return values;
  }

  int         get_max_allowed_packet();
  std::string quote(const char* in);

  unsigned    errnumber() { return ::mysql_errno(mysql_); }
  std::string error() { return ::mysql_error(mysql_); }

  void begin() { query("begin", false); }
  void rollback() { query("rollback", false); }

private:
  MYSQL* mysql_ = nullptr;
};

std::string quote_identifier(const std::string& identifier);

namespace impl {
// render integer value into buffer pre-filled with '0'
// doesn't work for negatives, but uses long for convenient interoperability
inline void stamp(char* s, long i) {
  do {
    *s-- = char(i % 10) + '0'; // NOLINT ptr arith
    i /= 10;
  } while (i > 0);
}
} // namespace impl

// much faster date format function "YYYY-MM-DD HH:MM:SS" (credit Howard Hinnant)
template <typename TimePointType>
std::string format_time_point(TimePointType tp) {
  if constexpr (!std::is_same_v<TimePointType, date::sys_days> &&
                !std::is_same_v<TimePointType, date::sys_seconds>) {
    static_assert(os::str::always_false<TimePointType>,
                  "do not know how to format this TimePointType");
  }

  auto today = floor<date::days>(tp);

  using impl::stamp;
  //                 yyyy-mm-dd
  std::string out = "0000-00-00";
  //                 0123456789
  if constexpr (std::is_same_v<TimePointType, date::sys_seconds>) {
    //     YYYY-MM-DD hh:mm:ss
    out = "0000-00-00 00:00:00";
    //     0123456789012345678
    date::hh_mm_ss hms{tp - today};
    stamp(&out[12], hms.hours().count());
    stamp(&out[15], hms.minutes().count());
    stamp(&out[18], hms.seconds().count());
  }

  date::year_month_day ymd = today;
  stamp(&out[3], int{ymd.year()});
  stamp(&out[6], unsigned{ymd.month()});
  stamp(&out[9], unsigned{ymd.day()});

  return out;
}

} // namespace mypp
