#include "qa_set.hpp"
#include "strutil.h"
#include "xos/console.hpp"
#include <algorithm>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <random>
#include <string>

bool qa_set::open(const std::string& filename) {
  _filename = filename;
  std::filesystem::path path(_filename);
  _lb_filename = (path.parent_path() / (".lb_" + path.filename().string())).string();
  _file.open(_filename);
  return _file.is_open();
}

bool qa_set::load() {
  _file.seekg(0);
  std::getline(_file, _stem); // read 1st line manually
  _file.seekg(0);             // and reset, then skip first line
  csv::CSVReader reader(_file, csv::CSVFormat().trim({' ', '\t'}).header_row(1));
  for (csv::CSVRow& row: reader) {
    qa qa;
    qa.load(row);
    _qas.push_back(qa);
  }
  return !_qas.empty();
}

void qa_set::run() {
  std::vector<qa> qas = _qas; // make a copy
  while (!(qas = ask_questions(qas)).empty())
    ;
}

std::vector<qa> qa_set::ask_questions(std::vector<qa>& qas) {
  using std::cout;

  xos::console::clear_screen();
  std::shuffle(qas.begin(), qas.end(), std::mt19937(std::random_device{}()));

  int         correct_count = 0;
  std::size_t total         = qas.size();
  cout << "Answer these " << total << " questions. Answer '?' to temporarily skip a question\n\n";
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
    if (!qas.empty()) cout << "\nRestarting with skipped questions...\n\n";
  }
  auto stop = std::chrono::system_clock::now();

  cout << "\n\n-----------------------------\n";
  cout << "You got " << correct_count << " out of " << total << " correct.\n\n";

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

void qa_set::add_to_lb(leader ld) {
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

void qa_set::load_lb() {
  _lb_file.open(_lb_filename);
  csv::CSVReader reader(_lb_file, csv::CSVFormat().trim({' ', '\t'}));
  _lb.clear();
  for (csv::CSVRow& row: reader) {
    leader ld;
    ld.load(row);
    _lb.push_back(ld);
  }
  _lb_file.close();
  std::sort(_lb.begin(), _lb.end(), std::greater<>());
}

void qa_set::write_lb() {
  if (!_lb.empty()) {
    _lb_file.open(_lb_filename, std::ios::out); // overwrite or create
    auto writer = csv::make_csv_writer(_lb_file);
    writer << std::make_tuple("name", "score", "time");
    for (const auto& ld: _lb) {
      ld.write(writer);
    }
    _lb_file.close();
  }
}

void qa_set::print_lb() {
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
