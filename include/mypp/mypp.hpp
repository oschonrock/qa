#pragma once

#include "date/date.h"
#include "fmt/core.h"
#include "os/str.hpp"
#include "os/tmp.hpp"
#include <cstddef>
#include <cstring>
#include <mysql.h>
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

  row                      fetch_row();
  unsigned                 num_fields() { return ::mysql_num_fields(myr); }
  std::vector<std::string> fieldnames();
  std::size_t*             lengths() {
    // is a direct return if we used mysql_use_result
    // so no point caching it
    return ::mysql_fetch_lengths(myr);
  }

  struct Iterator;
  Iterator begin();
  Iterator end();

private:
  mysql*     mysql;
  MYSQL_RES* myr;
};

namespace impl {

// adapted from fmt::detail
template <typename ReturnType>
constexpr ReturnType parse_nonnegative_int(const char* begin, const char* end,
                                           ReturnType error_value) noexcept {

  assert(begin != end && '0' <= *begin && *begin <= '9');
  std::uint64_t value = 0;
  std::uint64_t prev  = 0;
  const char*   p     = begin;
  do {
    prev  = value;
    value = value * 10 + std::uint64_t(*p - '0');
    ++p;
  } while (p != end && '0' <= *p && *p <= '9');
  auto num_digits = p - begin;
  if (num_digits <= std::numeric_limits<ReturnType>::digits10)
    return static_cast<ReturnType>(value);
  // Check for overflow. Will never happen here
  const auto max = static_cast<std::uint64_t>(std::numeric_limits<ReturnType>::max());
  return num_digits == std::numeric_limits<ReturnType>::digits10 + 1 &&
                 prev * 10ULL + std::uint64_t(p[-1] - '0') <= max
             ? static_cast<ReturnType>(value)
             : error_value;
}

// high peformance mysql date format parsing
template <typename TimePointType>
TimePointType parse_date_time(const char* s, std::size_t len) {
  static_assert(std::is_same_v<TimePointType, date::sys_days> ||
                    std::is_same_v<TimePointType, date::sys_seconds>,
                "don't know how to parse this timepoint");

  using date::year, date::month, date::day, std::chrono::hours, std::chrono::minutes,
      std::chrono::seconds;

  // fmt   YYYY-MM-DD HH:MM:SS  (time part only applies to date::sys_seconds)
  //       0123456789012345678

  if (len < 10) throw std::domain_error("not long enough to parse a date `" + std::string(s) + "`");

  date::year_month_day ymd = {year(parse_nonnegative_int(&s[0], &s[4], -1)),
                              month(parse_nonnegative_int(&s[5], &s[7], ~0U)),
                              day(parse_nonnegative_int(&s[8], &s[10], ~0U))};

  if (!ymd.ok()) throw std::domain_error("invalid date `" + std::string(s) + "`");

  auto date_tp = date::sys_days{ymd};

  if constexpr (std::is_same_v<TimePointType, date::sys_days>) {
    return date_tp;
  } else { // date::sys_seconds
    if (len < 19)
      throw std::domain_error("not long enough to parse a datetime `" + std::string(s) + "`");

    auto hrs  = hours(parse_nonnegative_int(&s[11], &s[13], ~0U));
    auto mins = minutes(parse_nonnegative_int(&s[14], &s[16], ~0U));
    auto secs = seconds(parse_nonnegative_int(&s[17], &s[19], ~0U));

    if (hrs.count() > 23 || mins.count() > 59 || secs.count() > 59)
      throw std::domain_error("invalid time in `" + std::string(s) + "`");

    return date_tp + hrs + mins + secs;
  }
}

// mysql/mariadb specific parsing
template <typename NumericType>
inline NumericType parse(const char* str, std::size_t len) {
  if constexpr (os::tmp::is_optional<NumericType>::value) {
    if (str == nullptr) return std::nullopt;

    using InnerType = std::remove_reference_t<decltype(std::declval<NumericType>().value())>;

    // special DATE and DATETIME null'ish values
    if constexpr (std::is_same_v<InnerType, date::sys_seconds>) {
      if (std::strcmp(str, "0000-00-00 00:00:00") == 0) return std::nullopt;
    } else if constexpr (std::is_same_v<InnerType, date::sys_days>) {
      if (std::strcmp(str, "0000-00-00") == 0) return std::nullopt;
    }

    return parse<InnerType>(str, len); // unwrap and recurse
  } else {
    if (str == nullptr)
      throw std::domain_error("requested type was not std::optional, but db returned NULL");

    if constexpr (std::is_same_v<NumericType, bool>) {
      return os::str::parse<long>(str) != 0; // db uses int{0} and int{1} for bool

    } else if constexpr (std::is_same_v<NumericType, date::sys_days> ||
                         std::is_same_v<NumericType, date::sys_seconds>) {

      return parse_date_time<NumericType>(str, len);
    } else {
      // delegate to general numeric formats
      return os::str::parse<NumericType>(str, len);
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

  [[nodiscard]] std::size_t len(unsigned idx) const { return rs->lengths()[idx]; }

  template <typename ValueType = std::string>
  [[nodiscard]] ValueType get(unsigned idx) const {
    // try numeric parsing by default
    return impl::parse<ValueType>((*this)[idx], len(idx));
  }

  // takes a copy in a std::string
  template <>
  [[nodiscard]] std::string get<std::string>(unsigned idx) const {
    return (*this)[idx]; // converting constructor
  }

  // access to the raw const char*, potentially for external parsing
  // beware lifetimes!
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
      if constexpr (os::tmp::has_push_back<ContainerType>::value)
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
inline constexpr void stamp(char* s, long i) {
  do {
    *s-- = static_cast<char>(i % 10) + '0'; // NOLINT narrowing
    i /= 10;
  } while (i > 0);
}
} // namespace impl

// much faster date format function "YYYY-MM-DD HH:MM:SS" (credit Howard Hinnant)
template <typename TimePointType>
std::string format_time_point(TimePointType tp) {
  static_assert(std::is_same_v<TimePointType, date::sys_days> ||
                    std::is_same_v<TimePointType, date::sys_seconds>,
                "do not know how to format this TimePointType");

  auto today = floor<date::days>(tp);

  using impl::stamp;
  //                 YYYY-MM-DD
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
