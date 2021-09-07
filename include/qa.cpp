#include "qa.hpp"
#include "strutil.h"
#include "xos/console.hpp"

bool qa::load(const csv::CSVRow& row) {
  question_ = row["question"].get<>();
  answer_   = row["answer"].get<>();
  return !question_.empty();
}

[[nodiscard]] qa_resp qa::ask(const std::string& stem) const {
  using std::string, std::cout;
  string prompt = stem + " " + question_ + "? ";
  cout << prompt;
  string ans;
  std::getline(std::cin, ans, '\n');
  int posx = static_cast<int>(prompt.size() + ans.size() + 3);

  xos::console::move_cursor_relative(posx, -1);
  
  trim(ans);
  if (ans == "?") {
    cout << "=> Skipped\n";
    return qa_resp::skip;
  }
  if (ans == answer_) {
    cout << "=> Correct\n";
    return qa_resp::correct;
  }
  string upper_ans    = toupper(ans);
  string upper_answer = toupper(answer_);
  if (upper_ans == upper_answer) {
    cout << "=> Correct, but check capitalisation: " << answer_ << "\n";
    return qa_resp::correct;
  }
  if (edit_distance(upper_ans, upper_answer) < 3) {
    cout << "=> Correct, but check spelling: " << answer_ << "\n";
    return qa_resp::correct;
  }
  cout << "=> Wrong, correct answer is: " << answer_ << '\n';
  return qa_resp::wrong;
}
