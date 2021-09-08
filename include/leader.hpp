#include "csv.hpp"
#include "fmt/format.h"
#include <compare>
#include <string>

class leader {
public:
  bool load(const csv::CSVRow& row) {
    name_  = row["name"].get<>();
    score_ = row["score"].get<int>();
    time_  = row["time"].get<double>();
    return !name_.empty();
  }

  static std::string time_to_string(double time) {
    int    minutes = static_cast<int>(time / 60);
    double seconds = time - (60 * minutes);
    return fmt::format("{:02d}:{:04.1f}", minutes, seconds);
  }

  std::partial_ordering operator<=>(leader const& rhs) const {
    if (auto c = score_ <=> rhs.score_; !std::is_eq(c)) return c;
    // must return partial due to this float comparison. Reversed for ascending order!
    return rhs.time_ <=> time_;
  }

  bool write(csv::CSVWriter<std::fstream>& writer) const {
    writer << std::make_tuple(name_, score_, time_);
    return true;
  }

  std::string name_;
  int         score_ = 0;
  double      time_  = 0;
};
