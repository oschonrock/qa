#include "os/algo.hpp"
#include "os/bch.hpp"
#include "os/fs.hpp"
#include "os/str.hpp"
#include "sha1/sha1.hpp"
#include <algorithm>
#include <array>
#include <compare>
#include <cstdint>
#include <cstdio>
#include <exception>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <optional>
#include <stdexcept>
#include <string>

unsigned make_nibble(char nibblechr) {
  int nibble = nibblechr - '0';
  if (nibble > 9) nibble = nibble - ('A' - '0') + 10;
  return static_cast<unsigned>(nibble);
}

struct password {
  password(std::ifstream& db, std::size_t pos) { // NOLINT initialization
    // the below is about 1.5x - 2x faster (but requires non-standard `cfile` wrapper)
    // std::FILE* db_cfile = os::fs::cfile(db);
    // std::fseek(db_cfile, static_cast<long>(pos * sizeof(password)), SEEK_SET);
    // std::fread(this, sizeof(*this), 1, db_cfile);
    db.seekg(static_cast<long>(pos * sizeof(password)));
    db.read(reinterpret_cast<char*>(this), sizeof(*this)); // NOLINT reinterpret_cast
  }

  // line must be an upppercase sha1 hexstr with optional ":123" appended (123 is the count).
  explicit password(const std::string& line) { // NOLINT initlialisation
    assert(hexstr.length() >= hash.size() * 2);
    // do this here in constr body and not in constr init list with functions call, because of this:
    // https://stackoverflow.com/questions/69427517/nrvo-of-stdarray-in-constructor-initialiser-list
    for (auto [i, b]: os::algo::enumerate(hash)) // note b is by reference!
      b = static_cast<std::byte>(make_nibble(line[2 * i]) << 4U | make_nibble(line[2 * i + 1]));

    if (line.size() > hash.size() * 2 + 1)
      count = os::str::parse_nonnegative_int(line.c_str() + hash.size() * 2 + 1,
                                             line.c_str() + line.size(), -1);
    else
      count = -1;
  }

  bool operator==(const password& rhs) const { return hash == rhs.hash; }

  std::strong_ordering operator<=>(const password& rhs) const { return hash <=> rhs.hash; }

  friend std::ostream& operator<<(std::ostream& os, const password& rhs) {
    os << std::setfill('0') << std::hex << std::uppercase;
    for (auto&& c: rhs.hash) os << std::setw(2) << static_cast<unsigned>(c);
    os << std::dec << ":" << rhs.count;
    return os;
  }

  std::array<std::byte, 20> hash;
  int32_t                   count; // be definitive about size
};

std::optional<password> search(password needle, std::ifstream& db, std::size_t db_size) {
  std::size_t count = db_size;
  std::size_t first = 0;
  {
    os::bch::Timer t("search");
    // lower_bound binary search algo
    while (count > 0) {
      std::size_t pos  = first;
      std::size_t step = count / 2;
      pos += step;
      password cur(db, pos);
      if (cur < needle) { // NOLINT weird nullptr warning
        first = ++pos;
        count -= step + 1;
      } else
        count = step;
    }
    if (first < db_size) {
      password found(db, first);
      if (found == needle) return found;
    }
  }
  return std::nullopt;
}

int main(int argc, char* argv[]) {
  std::ios_base::sync_with_stdio(false);

  // // convert text file to binary. left here for future updates
  // // std::getline is about 2x faster than `std::cin >> line` here
  // for (std::string line; std::getline(std::cin, line);) {
  //   password pw(line);
  //   // this is about 8x faster than std::cout.write(reinterpret_cast<char*>(&pw), sizeof(pw))
  //   std::fwrite(&pw, sizeof(pw), 1, stdout);
  // }
  // return 0;

  try {
    if (argc < 3)
      throw std::domain_error("USAGE: " + std::string(argv[0]) + " dbfile.bin plaintext_password");

    auto dbpath = std::filesystem::path(argv[1]);

    std::size_t db_file_size = std::filesystem::file_size(dbpath);
    if (db_file_size % sizeof(password) != 0)
      throw std::domain_error("db_file_size is not a multiple of the record size");
    std::size_t db_size = db_file_size / sizeof(password);

    auto db = std::ifstream(dbpath, std::ios::binary);
    if (!db.is_open())
      throw std::domain_error("cannot open db: " + std::string(dbpath));

    SHA1 sha1;
    sha1.update(argv[2]);
    password needle(sha1.final());

    std::cout << "needle = " << needle << "\n";
    auto found = search(needle, db, db_size);

    if (found)
      std::cout << "found  = " << *found << "\n";
    else
      std::cout << "not found\n";

  } catch (const std::exception& e) {
    std::cerr << "something went wrong: " << e.what() << "\n";
  }

  return EXIT_SUCCESS;
}
