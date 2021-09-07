#include "qa_set.hpp"
#include "fmt/include/fmt/core.h"
#include "strutil.h"
#include "xos/console.hpp"
#include <algorithm>
#include <chrono>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <random>
#include <string>

bool qa_set::open(const std::string& filename) {
  filename_ = filename;
  std::filesystem::path path(filename_);
  lb_filename_ = (path.parent_path() / (".lb_" + path.filename().string())).string();
  file_.open(filename_);
  return file_.is_open();
}

bool qa_set::load() {
  file_.seekg(0);
  std::getline(file_, stem_); // read 1st line manually
  file_.seekg(0);             // and reset, then skip first line
  csv::CSVReader reader(file_, csv::CSVFormat().trim({' ', '\t'}).header_row(1));
  for (csv::CSVRow& row: reader) {
    qa qa;
    qa.load(row);
    qas_.push_back(qa);
  }
  return !qas_.empty();
}

void qa_set::run() {
  std::vector<qa> qas = qas_; // make a copy
  while (!(qas = ask_questions(qas)).empty())
    ;
}

std::vector<qa> qa_set::ask_questions(std::vector<qa>& qas) {
  xos::console::clear_screen();
  std::shuffle(qas.begin(), qas.end(), std::mt19937(std::random_device{}()));

  int         correct_count = 0;
  std::size_t total         = qas.size();

  fmt::print("Answer these {:d} questions. Answer '?' to temporarily skip a question\n\n", total);

  std::vector<qa> wrong_qas;
  std::vector<qa> skipped_qas;
  auto            start = std::chrono::system_clock::now();
  while (!qas.empty()) {
    for (const auto& qa: qas) {
      switch (qa.ask(stem_)) {
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
    if (!qas.empty()) fmt::print("\nRestarting with skipped questions...\n\n");
  }
  auto   stop = std::chrono::system_clock::now();
  double time = static_cast<double>(
                    std::chrono::duration_cast<std::chrono::milliseconds>(stop - start).count()) /
                1000.0;

  fmt::print("\n\n-----------------------------\n");
  fmt::print("You got {:d} out of {:d} correct, in {:s}.\n\n", correct_count, total,
             leader::time_to_string(time));

  if (total == qas_.size()) { // this is the first run through. Leaderboard?
    load_lb();
    leader new_ld = {"", correct_count, time};
    add_to_lb(new_ld);
    write_lb();
  }

  fmt::print("Hit Enter to {:s}", wrong_qas.empty() ? "quit." : "proceed to wrong questons.");
  std::string enter;
  std::getline(std::cin, enter, '\n');
  return wrong_qas;
}

void qa_set::add_to_lb(leader ld) {
  if (lb_.size() < max_leaders || ld > lb_.back()) { // NOLINT bogus nullptr warning
    fmt::print("Congratulations! You have made the Top {:d} leaderboard!\n", max_leaders);
    fmt::print("Please enter your name: ");
    std::getline(std::cin, ld.name_, '\n');
    trim(ld.name_);
    lb_.insert(std::upper_bound(lb_.begin(), lb_.end(), ld, std::greater()), ld);
    if (lb_.size() > max_leaders) lb_.erase(lb_.begin() + max_leaders, lb_.end());
    print_lb();
  }
}

void qa_set::load_lb() {
  lb_file_.open(lb_filename_);
  csv::CSVReader reader(lb_file_, csv::CSVFormat().trim({' ', '\t'}));
  lb_.clear();
  for (csv::CSVRow& row: reader) {
    leader ld;
    ld.load(row);
    lb_.push_back(ld);
  }
  lb_file_.close();
  std::sort(lb_.begin(), lb_.end(), std::greater<>());
}

void qa_set::write_lb() {
  if (!lb_.empty()) {
    lb_file_.open(lb_filename_, std::ios::out); // overwrite or create
    auto writer = csv::make_csv_writer(lb_file_);
    writer << std::make_tuple("name", "score", "time");
    for (const auto& ld: lb_) {
      ld.write(writer);
    }
    lb_file_.close();
  }
}

void qa_set::print_lb() {
  if (!lb_.empty()) {
    fmt::print("\n\nLeaderboard\n\n");
    fmt::print("{:4s}   {:<20s}   {:>5s}   {:>7s}\n", "Rank", "Name", "Score", "Time");

    fmt::print("{:-<4s}   {:-<20s}   {:->5s}   {:->7s}\n", "", "", "", "");
    int counter = 0;
    for (const auto& ld: lb_) {
      counter++;
      fmt::print("{:4d}   {:<20s}   {:>5d}   {:>7s}\n", counter, ld.name_.c_str(), ld.score_,
                 leader::time_to_string(ld.time_).data());
    }
    fmt::print("\n\n");
  }
}
