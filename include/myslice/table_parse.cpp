#include "myslice.hpp"
#include "os/str.hpp"
#include <ctre.hpp>

namespace myslice {

void table::parse_fields() {
  for (auto&& line: get_create_lines()) {
    if (auto m = ctre::match<R"(^ +`([^`]+)` ([^ (,]+)(\(([0-9.,]+)\))?( ([^,]+))?,?$)">(line)) {

      auto& f = goc_field(m.get<1>().to_string());

      f.type         = m.get<2>().to_string();
      f.quoting_type = field::type_map().at(f.type);

      if (auto s = m.get<4>()) f.size = s.to_number();

      if (auto o = m.get<6>()) {
        auto options = o.to_string();
        f.options    = options;
        f.nullable   = !os::str::contains("NOT NULL", options);
      }

    } else if (auto m2 = ctre::search<R"(PRIMARY KEY +\(([^)]+)\))">(line)) {

      auto pks = os::str::explode(",", m2.get<1>().to_string());

      for (auto&& pk: pks) {
        os::str::trim(pk, "`");
        auto& pk_field = fields.at(pk);
        add_pk_field(pk_field);
      }
    }
  }
}

void table::parse_foreign_keys() {
  for (auto&& line: get_create_lines()) {
    if (os::str::contains("CONSTRAINT", line)) {
      if (auto m = ctre::search<R"(CONSTRAINT `[^`]+` FOREIGN KEY \(`([^`]+)`\) )"
                                R"(REFERENCES `([^`]+)` \(`([^`]+)`\))">(line)) {

        auto& local_field = fields.at(m.get<1>().to_string());

        auto& foreign_table = db->goc_table(m.get<2>().to_string()); // triggers recursion
        auto& foreign_field = foreign_table.fields.at(m.get<3>().to_string());

        auto& fk = add_foreign_key(local_field, foreign_field);

        for (auto m2: ctre::range<"( ON (DELETE|UPDATE) "
                                  "(RESTRICT|CASCADE|SET NULL|NO ACTION|SET DEFAULT))">(line)) {
          auto action = m2.get<2>().to_string();
          if (action == "DELETE")
            fk.ondelete = foreign_key::get_refoption(m2.get<3>().to_string());
          else if (action == "UPDATE")
            fk.onupdate = foreign_key::get_refoption(m2.get<3>().to_string());
        }
      } else {
        std::cerr << "Warning: Unparsable fk defintiion '" << line << "'\n";
      }
    }
  }
}

} // namespace myslice
