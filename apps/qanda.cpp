#include "qa_set.hpp"
#include <iostream>
#include <string>

int main(int argc, char* argv[]) {
  std::vector<std::string> args(argv, argv + argc);
  
  if (args.size() < 2) {
    std::cerr << "Usage: " << args[0] << " question_answer.csv\n";
    return EXIT_FAILURE;
  }
  qa_set qa_set;
  if (!qa_set.open(args[1])) {
    std::cerr << "Couldn't open: '" << args[1] << "'\n";
    return EXIT_FAILURE;
  }
  if (!qa_set.load()) {
    std::cerr << "No questions and answers found in : '" << args[1] << "'\n";
    return EXIT_FAILURE;
  }
  qa_set.run();
  return EXIT_SUCCESS;
}
