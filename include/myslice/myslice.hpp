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

void               conf_init(const std::string& filename = "");
const std::string& conf_get(const std::string& key);

class field;
class foreign_key;
class table;
class database;

mypp::mysql& con();

class field {
public:
  field(table& t, std::string fieldname) : table(&t), name(std::move(fieldname)) {}

  table*       table;
  foreign_key* fk = nullptr;
  std::string  name;

  // and int is 32bits on all modern platforms, which matches mysql INT(11)
  std::optional<std::unordered_set<int>> restricted_values;

  std::string type;

  enum class qtype { string, numeric };
  qtype quoting_type = qtype::string;

  std::optional<int>         size;
  std::optional<std::string> options;

  bool pk       = false;
  bool nullable = false;

  bool restrict(const std::unordered_set<int>& values);
  bool                 is_pk() const;
  std::string          sql_where_clause(const std::unordered_set<int>& values) const;
  std::string          quote(const char* unquoted) const;
  std::ostream&        vprint(std::ostream& os);
  friend std::ostream& operator<<(std::ostream& os, const field& f);
  static const std::unordered_map<std::string_view, qtype>& type_map();

  void set_expunge_orphans(bool val) { expunge_orphans_ = val; }
  bool get_expunge_orphans() const;

private:
  std::optional<bool> expunge_orphans_;
};

class foreign_key {
public:
  foreign_key(field& local, field& foreign) : local_field(local), foreign_field(foreign) {
    local.fk = this;
  }

  field& local_field;
  field& foreign_field;

  enum class refoption { restrict, cascade, setnull, noaction, setdefault };

  refoption ondelete = refoption::restrict;
  refoption onupdate = refoption::restrict;

  friend std::ostream& operator<<(std::ostream& os, const foreign_key& fk);
  static refoption     get_refoption(const std::string& s);
};

class table {
public:
  explicit table(database& database, std::string tablename)
      : db(&database), name(std::move(tablename)) {}

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
  field&       pk_field() const;
  foreign_key& add_foreign_key(field& local_field, field& foreign_field);

  void parse_fields();
  void parse_foreign_keys();

  using fmap = std::vector<std::pair<unsigned, field*>>;
  fmap field_map(mypp::result& rs);

  void limit_pks(const std::unordered_set<int>& pk_values, const std::string& trigger = "manual");
  void limit(const std::string& sql_limiting_clause, const std::string& trigger = "manual");
  void truncate() { limit_pks({}, "manual truncate"); }

  std::unordered_set<int> limited_pks(const std::string& sql_limiting_clause,
                                      const std::string& order_by = "",
                                      const std::string& limit    = "") const;

  void dump(std::ostream& os);

  std::ostream&        vprint(std::ostream& os);
  friend std::ostream& operator<<(std::ostream& os, const table& t);

  void set_expunge_orphans(bool val) { expunge_orphans_ = val; }
  bool get_expunge_orphans() const;

private:
  std::vector<std::string>& get_create_lines();
  std::vector<std::string>  create_lines;

  std::optional<bool> expunge_orphans_;

  void         add_pk_field(field& f);
  mypp::result pk_ltd_rs() const;
  bool         is_ltd_by_pks() const;

  void dump_create(std::ostream& os);
  void dump_data_prefix(std::ostream& os) const;
  void dump_data_postfix(std::ostream& os) const;
};

class database {
public:
  explicit database(std::string dbname) : name(std::move(dbname)) {}

  std::string name;

  std::unordered_map<std::string, table> tables;     // owns the tables
  std::vector<table*>                    table_list; // to keep insertion order
  unsigned                               restrict_count  = 0;
  bool                                   expunge_orphans = true;

  table&      add_table(const std::string& tablename);
  table&      goc_table(const std::string& tablename, bool force = false);
  void        parse_tables(); 
  void        dump(std::ostream& os);
  static void begin() { con().query("begin", false); }

  friend std::ostream& operator<<(std::ostream& os, const database& db);

private:
  static std::vector<std::string> tablenames();
};

} // namespace myslice
