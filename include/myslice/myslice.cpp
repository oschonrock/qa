#include "myslice.hpp"
#include "fmt/core.h"
#include "mypp/mypp.hpp"
#include "os/algo.hpp"
#include "os/str.hpp"
#include <cstdlib>
#include <fstream>
#include <iterator>
#include <sstream>
#include <stdexcept>
#include <string>
#include <unordered_map>

namespace myslice {

static std::unordered_map<std::string, std::string> conf; // NOLINT must be static non-const

void conf_init(const std::string& filename) {
  if (filename.length() == 0)
    throw std::logic_error("must provide config filename on first call to `conf()`");

  std::ifstream configfile(filename);
  if (!configfile.is_open())
    throw std::logic_error("could not open config file: tried `" + filename + "`.");

  for (std::string line; std::getline(configfile, line);) {
    os::str::trim(line);
    if (line.empty()) continue;
    auto pieces = os::str::explode("=", line);
    if (pieces.size() != 2)
      throw std::logic_error("illegal configfile syntax, please use `key=value` on each line");
    auto& key   = pieces[0];
    auto& value = pieces[1];
    os::str::trim(key);
    if (key.length() == 0)
      throw std::logic_error("config file key is empty on line: `" + line + "`");
    if (conf.contains(key))
      throw std::logic_error("duplicate key error in config file: `" + key + "`");
    os::str::trim(value);
    if (value.length() == 0)
      throw std::logic_error("config file value is empty on line: `" + line + "`");
    conf.emplace(std::move(key), std::move(value));
  }
}

const std::string& conf_get(const std::string& key) {
  auto it = conf.find(key);
  if (it == conf.end()) throw std::logic_error("no config value found for `" + key + "`\n");
  return it->second; // must exist and has lifetime of life of program, so ref is fine
}

mypp::mysql& con() {
  static auto con = []() {
    mypp::mysql my;
    my.connect(conf_get("db_host"), conf_get("db_user"), conf_get("db_pass"), conf_get("db_db"),
               static_cast<unsigned>(std::stoi(conf_get("db_port"))), conf_get("db_socket"));
    return my;
  }();
  return con;
}

// field.cpp
std::ostream& operator<<(std::ostream& os, const field& f) {
  os << f.table->name << "." << f.name;
  return os;
}

// use a std::string as tmp buffer, most numeric values are very small so this is fast
// and a std::string has to be created inside mypp::mysql::quote() anyway
std::string field::quote(const char* unquoted) const {
  if (unquoted == nullptr) {
    return "NULL";
  }
  switch (quoting_type) {
  case field::qtype::string:
    return con().quote(unquoted);
    break;
  case field::qtype::numeric:
    return unquoted;
    break;
  }
}

std::ostream& field::vprint(std::ostream& os) {
  std::string type_size;
  if (size)
    type_size = fmt::format("{:s}({:d})", type, *size);
  else
    type_size = fmt::format("{:s}", type);

  // clang-format off
  os << fmt::format("{:<25s}", name)
     << fmt::format(" {:2s}", pk ? "PK" : "")
     << fmt::format(" {:1s}", nullable ? "N" : "")
     << fmt::format(" {:1s}", quoting_type == field::qtype::string ? "Q" : "")
     << fmt::format(" {:<20}", type_size)
     << fmt::format(" {:s}", options.value_or("<no options>"));
  // clang-format on
  return os;
}

const std::unordered_map<std::string_view, field::qtype>& field::type_map() {
  const static std::unordered_map<std::string_view, qtype> map{
      {"char", qtype::string},     {"date", qtype::string},      {"datetime", qtype::string},
      {"time", qtype::string},     {"decimal", qtype::string},   {"float", qtype::numeric},
      {"int", qtype::numeric},     {"text", qtype::string},      {"mediumtext", qtype::string},
      {"longtext", qtype::string}, {"blob", qtype::string},      {"mediumblob", qtype::string},
      {"longblob", qtype::string}, {"smallint", qtype::numeric}, {"text", qtype::string},
      {"tinyint", qtype::numeric}, {"varchar", qtype::string},
  };
  return map;
}

// table.cpp
// some method definitions, which include use of ctre, have been seperated out for compilation speed

field& table::goc_field(const std::string& fieldname) {
  auto [it, was_inserted] = fields.try_emplace(fieldname, *this, fieldname);
  if (was_inserted) field_list.emplace_back(&it->second); // record insertion order
  return it->second;
}

foreign_key& table::add_foreign_key(field& local_field, field& foreign_field) {
  auto& fk = foreign_keys.emplace_back(local_field, foreign_field);
  foreign_field.table->referencing_fks.emplace_back(&fk); // link the other way
  return fk;
}

std::ostream& operator<<(std::ostream& os, const table& t) {
  os << "\n\nTable: " << t.name << "\n\n";
  for (auto&& f: t.field_list) os << *f << "\n";
  if (!t.foreign_keys.empty()) {
    os << "\n";
    for (auto&& fk: t.foreign_keys) os << fk << "\n";
  }
  if (!t.referencing_fks.empty()) {
    os << "\n";
    for (foreign_key* fk: t.referencing_fks) os << *fk << "\n";
  }
  return os;
}

std::vector<std::string>& table::get_create_lines() {
  if (create_lines.empty()) {
    auto rs      = con().query("show create table " + mypp::quote_identifier(name));
    create_lines = os::str::explode("\n", rs.fetch_row()[1]);
  }
  return create_lines;
}

void table::add_pk_field(field& f) {
  if (auto it = fields.find(f.name); it == fields.end())
    throw std::logic_error("Must add " + f.name + " to " + name +
                           " before adding it as a primary key");
  pk_fields.emplace_back(&f); // no attempt to dedupe!
  f.pk = true;
}

table::fmap table::field_map(mypp::result& rs) {
  auto fns = rs.fieldnames();
  fmap fm;
  fm.reserve(rs.num_fields());
  for (auto&& [i, fn]: os::algo::enumerate(fns)) fm.emplace_back(i, &fields.at(fn));
  return fm;
}

void table::dump(std::ostream& os) {
  std::cerr << "dumping `" << name << "`\n";
  // dumpCreate();
  // dumpDataPrefix();

  auto rs = con().query("select * from " + mypp::quote_identifier(name));
  auto fm = field_map(rs);

  std::string       sql_prefix   = "INSERT INTO " + mypp::quote_identifier(name) + " VALUES\n";
  bool              first        = true;
  std::int64_t      packet_count = 0;
  std::stringstream row_sql;
  for (auto&& row: rs) {
    if (first) {
      os << sql_prefix;
      packet_count = static_cast<std::int64_t>(sql_prefix.size());
    }

    row_sql.str(""); // allows reuse of malloc'd buffer
    row_sql.clear();
    row_sql << "(";
    for (auto&& [i, f]: fm) {
      row_sql << f->quote(row[i]); // NOLINT ptr arith.
      if (i == fm.size() - 1)
        row_sql << ")";
      else
        row_sql << ",";
    }

    if (packet_count + 2 + row_sql.tellp() < max_allowed_packet) { // fits
      if (!first) {
        os << ",\n";
        packet_count += 2;
      }
      os << row_sql.rdbuf();
      packet_count += row_sql.tellp();
    } else { // new query
      os << ";\n" << sql_prefix << row_sql.rdbuf();
      packet_count = static_cast<std::int64_t>(sql_prefix.size()) + row_sql.tellp();
    }
    first = false; // don't branch
  }
  if (!first) {
    os << ";\n"; // finish any packet which was started
  }

  // dumpDataPostfix();
  // std::cerr << " " . number_format(microtime(true) - start, 3) . "\n");
}

// foreign_key.cpp
foreign_key::refoption foreign_key::get_refoption(const std::string& s) {
  const static std::unordered_map<std::string, refoption> map = {
      {"RESTRICT", refoption::restrict},      {"CASCADE", refoption::cascade},
      {"SET NULL", refoption::setnull},       {"NO ACTION", refoption::noaction},
      {"SET DEFAULT", refoption::setdefault},
  };
  return map.at(s);
}

std::ostream& operator<<(std::ostream& os, const foreign_key& fk) {
  os << fk.local_field << " -> " << fk.foreign_field;
  return os;
}

// database.cpp
table& database::add_table(const std::string& tablename) {
  auto [it, was_inserted] = tables.try_emplace(tablename, *this, tablename);
  auto& t                 = it->second;
  if (was_inserted) table_list.emplace_back(&t); // record insertion order
  return t;
}

table& database::goc_table(const std::string& tablename, bool force) {
  if (force || !tables.contains(tablename)) {
    auto& t = add_table(tablename);
    t.parse_fields();
    t.parse_foreign_keys(); // this will recurse
    return t;
  }
  return tables.at(tablename);
}

void database::parse_tables() {
  for (auto&& tablename: tablenames()) goc_table(tablename); // this will recurse
  // fix order which was determined by the recursion
  std::sort(begin(table_list), end(table_list),
            [](table* a, table* b) { return a->name < b->name; }); // NOLINT nullptr??
}

void database::dump(std::ostream& os) {
  for (auto&& t: table_list) t->dump(os);
}

std::ostream& operator<<(std::ostream& os, const database& db) {
  for (auto&& t: db.table_list) os << *t;
  return os;
}

std::vector<std::string> database::tablenames() {
  std::vector<std::string> tablenames;

  auto rs = con().query("show tables");
  for (MYSQL_ROW row = rs.fetch_row(); row != nullptr; row = rs.fetch_row())
    tablenames.emplace_back(row[0]);
  return tablenames;
}

} // namespace myslice
