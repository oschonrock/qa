#pragma once

#include "mypp/mypp.hpp"
#include <cstdint>
#include <iostream>
#include <list>
#include <mysql.h>
#include <optional>
#include <ostream>
#include <stdexcept>
#include <string_view>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

namespace myslice {

void conf_init(const std::string& filename = "");
const std::string& conf_get(const std::string& key);

class table;

class foreign_key;

class field {
public:
  field(table& table, std::string fieldname) : table(&table), name(std::move(fieldname)) {}

  table*       table;
  foreign_key* fk = nullptr;
  std::string  name;

  std::unordered_set<uint64_t> restricted_values;
  std::unordered_set<uint64_t> allowed_keys;

  std::string type;

  enum class qtype { string, numeric };
  qtype quoting_type = qtype::string;

  std::optional<int>         size;
  std::optional<std::string> options;

  bool pk       = false;
  bool nullable = false;

  std::optional<bool> expunge_orphans;

  std::string          quote(const char* unquoted) const;
  std::ostream&        vprint(std::ostream& os);
  friend std::ostream& operator<<(std::ostream& os, const field& f);
  static const std::unordered_map<std::string_view, qtype>& type_map();
};

class database;

class table {
public:
  explicit table(database& db, std::string name) : db(&db), name(std::move(name)) {}

  database*   db;
  std::string name;

  std::unordered_map<std::string, field> fields;     // owns the fields
  std::vector<field*>                    field_list; // for keeping order
  std::vector<field*>                    pk_fields;  // for quick reference

  // using std::list because small and no invalidation, for below
  std::list<foreign_key> foreign_keys;
  // fks pointing at this table,
  std::vector<foreign_key*> referencing_fks;

  // get_or_create_field
  field&       goc_field(const std::string& fieldname);
  foreign_key& add_foreign_key(field& local_field, field& foreign_field);

  void parse_fields();
  void parse_foreign_keys();
  using fmap = std::vector<std::pair<unsigned, field*>>;

  fmap field_map(mypp::result& rs);
  void dump(std::ostream& os);

  friend std::ostream& operator<<(std::ostream& os, const table& t);

private:
  static constexpr std::int64_t max_allowed_packet = 16 * 1024 * 1024 - 1000;

  std::vector<std::string>& get_create_lines();
  std::vector<std::string>  create_lines;

  void add_pk_field(field& f);
};

class foreign_key {
public:
  foreign_key(field& local_field, field& foreign_field)
      : local_field(local_field), foreign_field(foreign_field) {
    local_field.fk = this;
  }

  field& local_field;
  field& foreign_field;

  enum class refoption { restrict, cascade, setnull, noaction, setdefault };

  refoption ondelete = refoption::restrict;
  refoption onupdate = refoption::restrict;

  friend std::ostream& operator<<(std::ostream& os, const foreign_key& fk);
  static refoption     get_refoption(const std::string& s);
};

class database {
public:
  explicit database(std::string dbname) : name(std::move(dbname)) {}

  std::string name;

  std::unordered_map<std::string, table> tables;     // owns the tables
  std::vector<table*>                    table_list; // to keep insertion order

  table& add_table(const std::string& tablename);
  table& goc_table(const std::string& tablename, bool force = false);
  void   parse_tables();
  void   dump(std::ostream& os);

  friend std::ostream& operator<<(std::ostream& os, const database& db);

private:
  static std::vector<std::string> tablenames();
};

} // namespace myslice
