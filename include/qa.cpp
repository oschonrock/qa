#include "qa.hpp"
#include "strutil.h"

bool qa::load(const csv::CSVRow& row) {
  _question = row["question"].get<>();
  _answer   = row["answer"].get<>();
  return !_question.empty();
}

[[nodiscard]] qa_resp qa::ask(const std::string& stem) const {
  std::string prompt = stem + " " + _question + "? ";
  std::cout << prompt;
  std::string ans;
  std::getline(std::cin, ans, '\n');
  std::size_t posx = prompt.size() + ans.size() + 3;
  std::cout << "\x1B[1A\x1B[" << posx << "C"; // move cursor to end of answer
  trim(ans);
  if (ans == "?") {
    std::cout << "=> Skipped\n";
    return qa_resp::skip;
  }
  if (ans == _answer) {
    std::cout << "=> Correct\n";
    return qa_resp::correct;
  }
  std::string upper_ans    = toupper(ans);
  std::string upper_answer = toupper(_answer);
  if (upper_ans == upper_answer) {
    std::cout << "=> Correct, but check capitalisation: " << _answer << "\n";
    return qa_resp::correct;
  }
  if (edit_distance(upper_ans, upper_answer) < 3) {
    std::cout << "=> Correct, but check spelling: " << _answer << "\n";
    return qa_resp::correct;
  }
  std::cout << "=> Wrong, correct answer is: " << _answer << '\n';
  return qa_resp::wrong;
}
