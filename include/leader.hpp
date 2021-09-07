#include "csv.hpp"
#include <compare>
#include <iomanip>
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
    int                minutes = static_cast<int>(time / 60);
    double             seconds = time - (60 * minutes);
    std::ostringstream buf;
    buf << std::setfill('0') << std::setw(2) << minutes << ':' << std::fixed << std::setw(4)
        << std::setprecision(1) << seconds;
    return buf.str();
  }

  std::partial_ordering operator<=>(leader const& rhs) const {
    if (std::strong_ordering c = score_ <=> rhs.score_; !std::is_eq(c)) return c;
    // must return partial due to this float comparison. Note it is reversed!
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
