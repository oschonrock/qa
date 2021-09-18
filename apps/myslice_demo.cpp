#include "conf/conf.hpp"
#include "fmt/core.h"
#include "myslice/myslice.hpp"
#include "os/bch.hpp"
#include <iostream>
#include <locale>
#include <stdexcept>
#include <string>
#include <unordered_set>
#include <vector>

int main(int argc, char* argv[]) {
  std::vector<std::string> args(argv, argv + argc);

  std::ios::sync_with_stdio(false); //  speed
  try {
    if (args.size() < 2) throw std::invalid_argument("missing org_id");

    const int org_id = std::stoi(args[1]);

    conf::init(args[0] + ".ini");
    myslice::database db("wcdb");
    using myslice::con, fmt::format;

    {
      os::bch::Timer t1("parse");
      db.parse_tables();
    }
    {
      os::bch::Timer t1("limit");
      con().begin(); // get a consistent view of db

      // by default don"t expunge orphans of nullable FKs (we end up removing too much)
      db.expunge_orphans = false;

      // cascade rules for FKs which are nullbable on the child side
      // this is deeply flawed now, because payment is not necessarily linked to an order
      db.tables.at("payment").fields.at("order_id").set_expunge_orphans(true);
      db.tables.at("psp_transaction").fields.at("payment_id").set_expunge_orphans(true);
      db.tables.at("organisation_account").fields.at("organisation_id").set_expunge_orphans(true);

      // tables we don"t want, ever
      db.tables.at("job").truncate();
      db.tables.at("target_organisation").truncate();
      db.tables.at("seo_ranking").truncate();
      db.tables.at("session").truncate();

      db.tables.at("organisation").limit_pks({org_id});

      // special many-to-many upwards cascade (plus superadmins)
      db.tables.at("member").limit_pks(con().single_column<std::unordered_set<int>>(format(
          "select member_id from member_to_organisation m2o join member m on m2o.member_id = m.id "
          "where m2o.organisation_id = {:d} or m.email like '%@webcollect.org.uk'",
          org_id)));

      // special case: 2 optional FKs
      db.tables.at("product").limit_pks(con().single_column<std::unordered_set<int>>(
          format("select p.id from product p left join event e on p.event_id = e.id where "
                 "e.organisation_id = {0:d} or p.organisation_id = {0:d}",
                 org_id)));

      // deal with double PK part1
      auto og_pk_field = db.tables.at("organisation_group").pk_field();
      if (og_pk_field.is_restricted()) {
        std::cerr << "Notice: special restrictions for ogm.organisation_group_id due to compound "
                     "FK which is not handled my myslice yet\n";
        db.tables.at("organisation_group_member")
            .fields.at("organisation_group_id")
            .restrict(*og_pk_field.get_restricted_values());
      }

      // deal with double PK part2
      auto o_pk_field = db.tables.at("organisation").pk_field();
      if (o_pk_field.is_restricted()) {
        std::cerr << "Notice: special restrictions for ogm.organisation_id due to compound FK "
                     "which is not handled my myslice yet\n";
        db.tables.at("organisation_group_member")
            .fields.at("organisation_id")
            .restrict(*o_pk_field.get_restricted_values());
      }
    }
    {
      os::bch::Timer t1("dump");
      db.dump(std::cout);
    }
    con().rollback(); // release the consistent view TX
  } catch (const std::invalid_argument& e) {
    std::cerr << "Bad command line Arguments: " << e.what() << "\n"
              << "USAGE: " << args[0] << " org_id\n";
    return EXIT_FAILURE;
  } catch (const std::logic_error& e) {
    std::cerr << "Logic error: " << e.what() << "\n";
    return EXIT_FAILURE;
  }
  return EXIT_SUCCESS;
}
