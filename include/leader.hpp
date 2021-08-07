#include "csv.hpp"
#include <string>
#include <iomanip>

class leader {
public:
  bool load(const csv::CSVRow& row) {
    _name  = row["name"].get<>();
    _score = row["score"].get<int>();
    _time  = row["time"].get<double>();
    return !_name.empty();
  }

  static std::string time_to_string(double time) {
    int minutes = static_cast<int>(time / 60);
    double seconds = time - (60 * minutes);
    std::ostringstream buf;
    buf << std::setfill('0') << std::setw(2) << minutes << ':'
        << std::fixed << std::setw(4) << std::setprecision(1) << seconds;
    return buf.str();
  }
  
  bool operator<(const leader& other) const {
    return _score < other._score || (_score == other._score && _time > other._time);
  }
  bool operator==(const leader& other) const {
    const double epsilon = 0.01;
    return _score == other._score && std::fabs(_time - other._time) < epsilon;
  }
  bool operator>(const leader& other) const { return other < *this; }
  bool operator!=(const leader& other) const { return !(*this == other); }
  bool operator<=(const leader& other) const { return !(*this > other); }
  bool operator>=(const leader& other) const { return !(*this < other); }

  bool write(csv::CSVWriter<std::fstream>& writer) const {
    writer << std::make_tuple(_name, _score, _time);
    return true;
  }

  std::string _name;
  int         _score = 0;
  double      _time  = 0;
};
