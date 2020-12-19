#pragma once

#include "csv.hpp"
#include "strutil.h"
#include <algorithm>
#include <bits/c++config.h>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <random>
#include <string>
#include <vector>

template <typename T>
void shuffle(std::vector<T>& v) {
  std::random_device seed;
  std::mt19937       prng(seed());
  std::shuffle(v.begin(), v.end(), prng);
}

enum class qa_resp { wrong, correct, skip };

class qa {
public:
  bool load(std::istream& stream) {
    if (!std::getline(stream, _question, ',') || !std::getline(stream, _answer, '\n')) return false;
    trim(_answer);
    trim(_question);
    return !_question.empty() && !_answer.empty();
  }

  [[nodiscard]] qa_resp ask(const std::string& stem) const {
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

  std::string _question;
  std::string _answer;
};

class leader {
public:
  bool load(csv::CSVRow& row) {
    _name  = row[0].get<>();
    _score = row[1].get<int>();
    _time  = row[2].get<double>();
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

  bool write(csv::CSVWriter<std::fstream>& writer) {
    writer << std::make_tuple(_name, _score, _time);
    return true;
  }

  std::string _name;
  int         _score = 0;
  double      _time  = 0;
};

class qa_set {
public:
  bool open(const std::string& filename) {
    _filename = filename;
    std::filesystem::path path(_filename);
    _lb_filename = (path.parent_path() / (".lb_" + path.filename().string())).string();
    _file.open(_filename);
    return _file.is_open();
  }
  bool load() {
    std::getline(_file, _stem, '\n');
    qa qa;
    while (qa.load(_file)) _qas.push_back(qa);
    return !_qas.empty();
  }
  void run() {
    load_lb();
    print_lb();
    return;
    std::vector<qa> qas = _qas; // make a copy
    while (!(qas = ask_questions(qas)).empty())
      ;
  }

private:
  std::vector<qa>     _qas;
  std::vector<leader> _lb;
  std::string         _stem;
  std::string         _filename;
  std::string         _lb_filename;
  std::ifstream       _file;
  std::fstream        _lb_file;

  std::vector<qa> ask_questions(std::vector<qa>& qas) {
    std::cout << "\x1B[2J\x1B[H"; // clear screen

    shuffle(qas);
    int         correct_count = 0;
    std::size_t total         = qas.size();
    std::cout << "Answer these " << total
              << " questions. Answer '?' to temporarily skip a question\n\n";
    std::vector<qa> wrong_qas;
    std::vector<qa> skipped_qas;
    auto            start = std::chrono::system_clock::now();
    while (!qas.empty()) {
      for (const auto& qa: qas) {
        switch (qa.ask(_stem)) {
        case qa_resp::correct:
          correct_count++;
          break;
        case qa_resp::wrong:
          wrong_qas.push_back(qa);
          break;
        case qa_resp::skip:
          skipped_qas.push_back(qa);
          break;
        }
      }
      qas = skipped_qas;
      skipped_qas.clear();
      if (!qas.empty()) std::cout << "\nRestarting with skipped questions...\n\n";
    }
    auto stop = std::chrono::system_clock::now();

    std::cout << "\n\n-----------------------------\n";
    std::cout << "You got " << correct_count << " out of " << total << " correct.\n\n";

    if (total == _qas.size()) { // this is the first run through. Leaderboard?
      int duration_ms = std::chrono::duration_cast<std::chrono::milliseconds>(stop - start).count();
      load_lb();
      leader new_ld = {"", correct_count, duration_ms / 1000.0};
      add_to_lb(new_ld);
      write_lb();
    }

    std::cout << "Hit Enter to " << (wrong_qas.empty() ? "quit." : "proceed to wrong questons.");
    std::string enter;
    std::getline(std::cin, enter, '\n');
    return wrong_qas;
  }

  const std::size_t max_leaders = 10;

  void add_to_lb(leader ld) {
    if (_lb.size() < max_leaders || ld > _lb.back()) {
      std::cout << "Congratulations! You have made the Top " << max_leaders << " leaderboard!\n";
      std::cout << "Please enter your name: ";
      std::getline(std::cin, ld._name, '\n');
      trim(ld._name);
      _lb.insert(std::upper_bound(_lb.begin(), _lb.end(), ld, std::greater<>()), ld);
      if (_lb.size() > max_leaders) _lb.erase(_lb.begin() + max_leaders, _lb.end());
      print_lb();
    }
  }
  void load_lb() {
    _lb_file.open(_lb_filename);
    csv::CSVReader reader(_lb_file, csv::CSVFormat().trim({' ', '\t'}).no_header());
    _lb.clear();
    for (csv::CSVRow& row: reader) {
      leader ld;
      ld.load(row);
      _lb.push_back(ld);
    }
    _lb_file.close();
    std::sort(_lb.begin(), _lb.end(), std::greater<>());
  }

  void write_lb() {
    if (!_lb.empty()) {
      _lb_file.open(_lb_filename, std::ios::out); // overwrite or create
      auto writer = csv::make_csv_writer(_lb_file);
      // writer << std::make_tuple("Name", "Score", "Time");
      for (auto ld: _lb) {
        ld.write(writer);
      }
      _lb_file.close();
    }
  }
  void print_lb() {
    if (!_lb.empty()) {
      std::cout << "\n\nLeaderboard\n\n";
      printf("%4s   %-20s   %5s   %7s\n", "Rank", "Name", "Score", "Time"); // NOLINT
      std::cout << std::string(4, '-') << "   " << std::string(20, '-') << "   "
                << std::string(5, '-') << "   " << std::string(7, '-') << '\n';
      int counter = 0;
      for (const auto& ld: _lb) {
        counter++;
        printf("%4d   %-20s   %5d   %02d:%04.1f\n", counter, ld._name.c_str(), ld._score, // NOLINT
               static_cast<int>(ld._time / 60),
               static_cast<int>(ld._time) % 60 + ld._time - static_cast<int>(ld._time));
      }
      std::cout << "\n\n";
    }
  }
};
