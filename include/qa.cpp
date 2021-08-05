#include "qa.hpp"
#include "strutil.h"
#include "xos/console.hpp"

bool qa::load(const csv::CSVRow& row) {
  _question = row["question"].get<>();
  _answer   = row["answer"].get<>();
  return !_question.empty();
}

[[nodiscard]] qa_resp qa::ask(const std::string& stem) const {
  using std::string, std::cout;
  string prompt = stem + " " + _question + "? ";
  cout << prompt;
  string ans;
  std::getline(std::cin, ans, '\n');
  std::size_t posx = prompt.size() + ans.size() + 3;

  xos::console::move_cursor_relative(posx, -1);
  
  trim(ans);
  if (ans == "?") {
    cout << "=> Skipped\n";
    return qa_resp::skip;
  }
  if (ans == _answer) {
    cout << "=> Correct\n";
    return qa_resp::correct;
  }
  string upper_ans    = toupper(ans);
  string upper_answer = toupper(_answer);
  if (upper_ans == upper_answer) {
    cout << "=> Correct, but check capitalisation: " << _answer << "\n";
    return qa_resp::correct;
  }
  if (edit_distance(upper_ans, upper_answer) < 3) {
    cout << "=> Correct, but check spelling: " << _answer << "\n";
    return qa_resp::correct;
  }
  cout << "=> Wrong, correct answer is: " << _answer << '\n';
  return qa_resp::wrong;
}
