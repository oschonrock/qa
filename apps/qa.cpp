#include "qa.h"
#include "strutil.h"
#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>

int main(int argc, char** argv) {
  std::ifstream f;
  if (argc < 2) {
    std::cerr << "Usage: " << argv[0] << " question_answer.csv\n";
    return EXIT_FAILURE;
  }
  qa_set qa_set;
  if (!qa_set.open(argv[1])) {
    std::cerr << "Couldn't open: '" << argv[1] << "'\n";
    return EXIT_FAILURE;
  }
  if (!qa_set.load()) {
    std::cerr << "No questions and answers found in : '" << argv[1] << "'\n";
    return EXIT_FAILURE;
  }
  qa_set.run();
  return EXIT_SUCCESS;
}
