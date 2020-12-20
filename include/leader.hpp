#include "csv.hpp"
#include <string>

class leader {
public:
  bool load(csv::CSVRow& row) {
    _name  = row["name"].get<>();
    _score = row["score"].get<int>();
    _time  = row["time"].get<double>();
    return !_name.empty();
  }

  bool operator<(const leader& other) const {
    return _score < other._score || (_score == other._score && _time > other._time);
  }
  bool operator>(const leader& other) const { return other < *this; }
  bool operator==(const leader& other) const {
    const double epsilon = 0.01;
    return _score == other._score && std::fabs(_time - other._time) < epsilon;
  }
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
