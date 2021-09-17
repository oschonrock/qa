#include "myslice.hpp"
#include "fmt/chrono.h"
#include "fmt/core.h"
#include "mypp/mypp.hpp"
#include "os/algo.hpp"
#include "os/str.hpp"
#include <cstdint>
#include <cstdlib>
#include <fstream>
#include <iterator>
#include <optional>
#include <ostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace myslice {

static std::unordered_map<std::string, std::string> conf; // NOLINT must be static non-const

using mypp::quote_identifier;

void conf_init(const std::string& filename) {
  if (filename.length() == 0)
    throw std::logic_error("must provide config filename on first call to `conf()`");

  std::ifstream configfile(filename);
  if (!configfile.is_open())
    throw std::logic_error("could not open config file: tried `" + filename + "`.");

  for (std::string line; std::getline(configfile, line);) {
    os::str::trim(line);
    if (line.empty()) continue;
    auto pos = line.find('=');
    if (pos == std::string::npos)
      throw std::logic_error("illegal configfile syntax, please use `name=value` on each line");
    auto key   = line.substr(0, pos);
    auto value = line.substr(pos + 1);

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
  auto it = conf.find(key); // using .contains() would mean 2 hash lookups
  if (it == conf.end()) throw std::logic_error("no config value found for `" + key + "`\n");
  return it->second; // must exist and has lifetime of life of program, so ref is fine
}

mypp::mysql& con() {
  static auto con = []() {
    mypp::mysql my;
    // clang-format off
    my.connect(conf_get("db_host"),
               conf_get("db_user"),
               conf_get("db_pass"),
               conf_get("db_db"),
               static_cast<unsigned>(std::stoi(conf_get("db_port"))),
               conf_get("db_socket"));
    // clang-format on
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

  if (unquoted == nullptr) return "NULL";

  if (nullable && fk != nullptr && fk->foreign_field.restricted_values_ &&
      !fk->foreign_field.restricted_values_->contains(os::str::parse<int>(unquoted)))
    return "NULL"; // maintain referential intergrity

  switch (quoting_type) {
  case field::qtype::string:
    return con().quote(unquoted);
  case field::qtype::numeric:
    return unquoted;
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
  os << t.name;
  return os;
}

std::ostream& table::vprint(std::ostream& os) {
  os << "\n\nTable: " << name << "\n\n";
  for (auto&& f: field_list) os << *f << "\n";
  if (!foreign_keys.empty()) {
    os << "\n";
    for (auto&& fk: foreign_keys) os << fk << "\n";
  }
  if (!referencing_fks.empty()) {
    os << "\n";
    for (foreign_key* fk: referencing_fks) os << *fk << "\n";
  }
  return os;
}

const std::vector<std::string>& table::get_create_lines() {
  if (create_lines.empty()) {
    auto ct = con().single_value<std::string>("show create table " + quote_identifier(name), 1);
    create_lines = os::str::explode("\n", ct);
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

field& table::pk_field() const {
  if (pk_fields.size() != 1)
    throw std::logic_error("must have exactly one pk field to call mypp::table::get_pk_field()");
  return *pk_fields[0];
}

void table::limit_pks(const std::unordered_set<int>& pk_values, const std::string& trigger) {
  ++db->restrict_count;
  std::cerr << "Notice: Table: " << *this << ": " << db->restrict_count << ": limited to "
            << pk_values.size() << " rows. Trigger: " << trigger << "\n";

  auto& pk = pk_field();
  if (pk.restrict(pk_values))
    // only recheck the foreign keys if we have been asked to by Field::restrict()
    for (auto&& fk: referencing_fks) fk->local_field.restrict(pk_values);

  // if the recursion above has succeeded then this table now has limited records
}

bool field::is_pk() const {
  return std::find(table->pk_fields.begin(), table->pk_fields.end(), this) !=
         table->pk_fields.end();
}

std::unordered_set<int> intersect(const std::unordered_set<int>& s1,
                                  const std::unordered_set<int>& s2) {
  auto [l, r] = [&]() {
    if (s1.size() < s2.size()) return std::tie(s1, s2);
    return std::tie(s2, s1);
  }();

  std::unordered_set<int> result;
  result.reserve(l.size());

  for (const auto& v: l)
    if (auto i = r.find(v); i != r.end()) result.insert(v);

  return result;
}

std::string field::sql_where_clause(const std::unordered_set<int>& values) const {
  if (values.empty()) return " false ";
  std::stringstream s;
  s << quote_identifier(name) << " IN (" << os::str::join(values, ",") << ") ";
  return s.str();
}

bool field::restrict(const std::unordered_set<int>& values) {
  if (is_pk()) {
    if (restricted_values_.has_value()) {
      // is already restricted..merging and reporting whether we need to re-recurse
      auto new_restricted_values = intersect(*restricted_values_, values);

      if (new_restricted_values.size() == restricted_values_->size())
        return false; // not changed, stop recursing

      restricted_values_ = std::move(new_restricted_values);
    } else {
      // first time a restriction is being applied
      restricted_values_ = values;
    }
  } else {
    // convert this non-primary key restriction into a pk one
    // by recursing

    // TODO(oliver): not yet dealing with FKs which are nullable on
    // the child side have tried adding "or is null" below, but this
    // recurses and grows the parent object sets need some extra check
    // to "not grow the allowable PK set under certain circumstances".
    // right now this will just give us a smaller db than perhaps
    // intended, which is not all bad
    if (nullable) {
      // if we we left this, it would be set NULL during dump,
      // effectively creating an orphan. By default we will remove
      // orphans there is a system of db/table/field defaults to
      // control this behaviour
      if (get_expunge_orphans()) {
        table->limit(sql_where_clause(values), os::str::stringify(*this));
      }
    } else {
      // if no option for null we really have to continue recursing
      table->limit(sql_where_clause(values), os::str::stringify(*this));
    }
  }
  return true;
}

bool field::get_expunge_orphans() const {
  return expunge_orphans_.value_or(table->get_expunge_orphans());
}

bool table::get_expunge_orphans() const { return expunge_orphans_.value_or(db->expunge_orphans); }

std::unordered_set<int> table::limited_pks(const std::string& sql_limiting_clause,
                                           const std::string& order_by,
                                           const std::string& limit) const {
  auto pk = pk_field();

  std::string sql = "select " + quote_identifier(pk.name) + " from " + quote_identifier(name) +
                    " where " + sql_limiting_clause;

  if (!order_by.empty()) sql += " order by " + order_by;
  if (!limit.empty()) sql += " limit " + limit;

  return con().single_column<std::unordered_set<int>>(sql);
}

void table::limit(const std::string& sql_limiting_clause, const std::string& trigger) {
  try {
    auto pk_values = limited_pks(sql_limiting_clause);
    limit_pks(pk_values, trigger);
  } catch (const std::logic_error& e) {
    std::cerr << "Warning: Table: " << os::str::stringify(*this)
              << ": could not convert sql_limiting_clause = '" << sql_limiting_clause
              << "' into a set of limited PKs. Table has more than 1 PK? " << e.what() << "\n";
  }
}

bool table::is_ltd_by_pks() const {
  try {
    auto& pk = pk_field();
    return pk.is_restricted();
  } catch (const std::logic_error& e) {
    // unable to get a singular PK field, so a restriction is not supported and hence cannot be
    // restricted
    return false;
  }
}

mypp::result table::pk_ltd_rs() const {
  std::string sql = "select * from " + quote_identifier(name);

  if (is_ltd_by_pks()) {
    auto& pk = pk_field();
    sql += " where " + pk.sql_where_clause(*pk.get_restricted_values());
  }
  return con().query(sql);
}

void table::dump(std::ostream& os) {
  std::cerr << fmt::format("dumping `{:25s}`", name);

  dump_create(os);
  dump_data_prefix(os);

  std::string  sql_prefix         = "INSERT INTO " + quote_identifier(name) + " VALUES\n";
  bool         first              = true;
  std::int64_t packet_count       = 0;
  int          max_allowed_packet = con().get_max_allowed_packet() - 1'000;

  auto rs = pk_ltd_rs();
  auto fm = field_map(rs);

  std::stringstream row_sql;
  std::int64_t      rowcount = 0;
  for (auto&& row: rs) {
    ++rowcount;
    if (first) {
      os << sql_prefix;
      packet_count = static_cast<std::int64_t>(sql_prefix.size());
    }

    row_sql.str(""); // allows reuse of malloc'd buffer
    row_sql.clear();
    row_sql << "(";
    for (auto&& [i, f]: fm) {
      row_sql << f->quote(row[i]);
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
  std::cerr << fmt::format("{:12d} Rows", rowcount) << "\n";

  dump_data_postfix(os);
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
  std::sort(std::begin(table_list), std::end(table_list),
            [](table* a, table* b) { return a->name < b->name; }); // NOLINT nullptr??
}

void database::dump(std::ostream& os) {
  // clang-format off
    os << R"(-- Myslice Dump
--
-- Host: )" << conf_get("db_host") << R"(    Database: )" << conf_get("db_db") << R"(
-- ------------------------------------------------------
-- Server version )" << con().single_value<std::string>("show variables like 'version'", 1) << R"(

/*!40101 SET @OLD_CHARACTER_SET_CLIENT=@@CHARACTER_SET_CLIENT */;
/*!40101 SET @OLD_CHARACTER_SET_RESULTS=@@CHARACTER_SET_RESULTS */;
/*!40101 SET @OLD_COLLATION_CONNECTION=@@COLLATION_CONNECTION */;
/*!40101 SET NAMES utf8 */;
/*!40103 SET @OLD_TIME_ZONE=@@TIME_ZONE */;
/*!40103 SET TIME_ZONE='+00:00' */;
/*!40014 SET @OLD_UNIQUE_CHECKS=@@UNIQUE_CHECKS, UNIQUE_CHECKS=0 */;
/*!40014 SET @OLD_FOREIGN_KEY_CHECKS=@@FOREIGN_KEY_CHECKS, FOREIGN_KEY_CHECKS=0 */;
/*!40101 SET @OLD_SQL_MODE=@@SQL_MODE, SQL_MODE='NO_AUTO_VALUE_ON_ZERO' */;
/*!40111 SET @OLD_SQL_NOTES=@@SQL_NOTES, SQL_NOTES=0 */;
)";
  // clang-format on

  for (auto&& t: table_list) t->dump(os);

  // clang-format off
  os << R"(/*!40103 SET TIME_ZONE=@OLD_TIME_ZONE */;

/*!40101 SET SQL_MODE=@OLD_SQL_MODE */;
/*!40014 SET FOREIGN_KEY_CHECKS=@OLD_FOREIGN_KEY_CHECKS */;
/*!40014 SET UNIQUE_CHECKS=@OLD_UNIQUE_CHECKS */;
/*!40101 SET CHARACTER_SET_CLIENT=@OLD_CHARACTER_SET_CLIENT */;
/*!40101 SET CHARACTER_SET_RESULTS=@OLD_CHARACTER_SET_RESULTS */;
/*!40101 SET COLLATION_CONNECTION=@OLD_COLLATION_CONNECTION */;
/*!40111 SET SQL_NOTES=@OLD_SQL_NOTES */;

-- Dump completed on )"
     << fmt::format("{:%Y-%m-%d %H:i:s}", fmt::localtime(std::time(nullptr))) << R"(
)";
  // clang-format on
}

std::ostream& operator<<(std::ostream& os, const database& db) {
  for (auto&& t: db.table_list) os << *t;
  return os;
}

std::vector<std::string> database::tablenames() {
  return con().single_column<std::vector<std::string>>("show tables");
}

void table::dump_create(std::ostream& os) {
  // clang-format off
  os << R"(
--
-- Table structure for table )" << quote_identifier(name) <<  R"(
--

DROP TABLE IF EXISTS )" << quote_identifier(name) <<  R"(;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
)";
  // clang-format on
  os << os::str::join(get_create_lines(), "\n") << ";\n"
     << R"(/*!40101 SET character_set_client = @saved_cs_client */;)"
     << "\n";
}

void table::dump_data_prefix(std::ostream& os) const {
  // clang-format off
  os << R"(
--
-- Dumping data for table )" << quote_identifier(name) <<  R"(
--

LOCK TABLES )" << quote_identifier(name) << R"( WRITE;
/*!40000 ALTER TABLE )" << quote_identifier(name) << R"( DISABLE KEYS */;
)";
  // clang-format on
}

void table::dump_data_postfix(std::ostream& os) const {
  // clang-format off
  os << R"(/*!40000 ALTER TABLE )" << quote_identifier(name) << R"( ENABLE KEYS */;
UNLOCK TABLES;
)";
  // clang-format on
}

} // namespace myslice
