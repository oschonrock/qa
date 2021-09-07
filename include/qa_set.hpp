#pragma once

#include "qa.hpp"
#include "leader.hpp"

class qa_set {
public:
  bool open(const std::string& filename);
  bool load();
  void run();

private:
  std::vector<qa>     qas_;
  std::vector<leader> lb_;
  std::string         stem_;
  std::string         filename_;
  std::string         lb_filename_;
  std::ifstream       file_;
  std::fstream        lb_file_;

  std::vector<qa> ask_questions(std::vector<qa>& qas);

  static constexpr std::size_t max_leaders = 10;

  void add_to_lb(leader ld);
  void load_lb();
  void write_lb();
  void print_lb();
};
