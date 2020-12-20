#pragma once

#include "qa.h"
#include "leader.hpp"

class qa_set {
public:
  bool open(const std::string& filename);
  bool load();
  void run();

private:
  std::vector<qa>     _qas;
  std::vector<leader> _lb;
  std::string         _stem;
  std::string         _filename;
  std::string         _lb_filename;
  std::ifstream       _file;
  std::fstream        _lb_file;

  std::vector<qa> ask_questions(std::vector<qa>& qas);

  const std::size_t max_leaders = 10;

  void add_to_lb(leader ld);
  void load_lb();
  void write_lb();
  void print_lb();
};
