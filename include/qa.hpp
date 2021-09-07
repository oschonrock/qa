#pragma once

#include "csv.hpp"
#include <iostream>
#include <string>

enum class qa_resp { wrong, correct, skip };

class qa {
public:
  bool load(const csv::CSVRow& row);

  [[nodiscard]] qa_resp ask(const std::string& stem) const;

  friend std::ostream& operator<<(std::ostream& os, const qa& qa) {
    os << qa.question_ << ": " << qa.answer_ << "\n";
    return os;
  }

private:
  std::string question_;
  std::string answer_;
};
