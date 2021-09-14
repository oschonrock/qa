#include "myslice/myslice.hpp"
#include "os/bch.hpp"
#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>

int main(int argc, char* argv[]) {
  std::vector<std::string> args(argv, argv+argc);
  
  std::ios::sync_with_stdio(false); //  speed
  try {
    myslice::conf_init(args[0] + ".ini");
    myslice::database db("wcdb");
    {
      os::bch::Timer t1("parse");
      db.parse_tables();
    }
    {
      os::bch::Timer t1("dump");
      db.dump(std::cout);
      // db.tables.at("virtual_value").dump(std::cout);
    }

  } catch (const std::logic_error& e) {
    std::cerr << "Logic error: " << e.what() << "\n";
    return EXIT_FAILURE;
  }
  return EXIT_SUCCESS;
}
