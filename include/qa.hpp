#pragma once

#include "csv.hpp"
#include "strutil.h"
#include <string>

enum class qa_resp { wrong, correct, skip };

class qa {
public:
  bool load(const csv::CSVRow& row);

  [[nodiscard]] qa_resp ask(const std::string& stem) const;

  friend std::ostream& operator<<(std::ostream& os, const qa& qa) {
    os << qa._question << ": " << qa._answer << "\n";
    return os;
  }

private:
  std::string _question;
  std::string _answer;
};
