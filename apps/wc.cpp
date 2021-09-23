#include "conf/conf.hpp"
#include "date/date.h"
#include "fmt/core.h"
#include "mypp/mypp.hpp"
#include "os/bch.hpp"
#include <algorithm>
#include <chrono>
#include <filesystem>
#include <iostream>
#include <numeric>
#include <string>
#include <unordered_map>
#include <vector>

mypp::mysql& db() {
  static auto db = []() {
    mypp::mysql db;
    db.connect(conf::get_or("db_host", "localhost"), conf::get("db_user"), conf::get("db_pass"),
               conf::get("db_db"), conf::get_or<unsigned>("db_port", 0U),
               conf::get_or("db_socket", ""));

    auto charset = conf::get_or("db_charset", "utf8");
    db.set_character_set(charset);

    std::cerr << "Notice: Connected to " << conf::get("db_db") << " on " << db.get_host_info()
              << " using " << charset << "\n";
    return db;
  }();
  return db;
}

struct member {
  std::string                      firstname;
  std::string                      lastname;
  std::string                      email;
  std::optional<date::sys_days>    dob;
  std::optional<date::sys_seconds> date_of_last_logon;
  std::optional<date::sys_seconds> created_at;
  std::optional<date::sys_seconds> updated_at;
  int                              email_failure_count;
  bool                             invalid;
  std::optional<date::sys_seconds> invalidated_time;

  friend std::ostream& operator<<(std::ostream& os, const member& m) {
    using mypp::format_time_point;

    os << m.firstname << " " << m.lastname << " <" << m.email << ">\n";
    if (m.dob) os << "dob=" << format_time_point(m.dob.value()) << "\n";
    if (m.date_of_last_logon)
      os << "date_of_last_logon=" << format_time_point(m.date_of_last_logon.value()) << "\n";
    if (m.created_at) os << "created_at=" << format_time_point(m.created_at.value()) << "\n";
    if (m.updated_at) os << "updated_at=" << format_time_point(m.updated_at.value()) << "\n";
    os << "email_failure_count=" << m.email_failure_count << "\n";
    os << "invalid=" << std::boolalpha << m.invalid << "\n";
    if (m.invalidated_time)
      os << "invalidated_time=" << format_time_point(m.invalidated_time.value());
    os << "\n";
    return os;
  }
};

std::vector<member> load_members(int limit = 10'000) {
  std::vector<member> members;

  for (auto&& r: db().query("select "
                            "  firstname, "
                            "  lastname, "
                            "  email, "
                            "  dob, "
                            "  date_of_last_logon, "
                            "  created_at, "
                            "  updated_at, "
                            "  email_failure_count, "
                            "  invalid, "
                            "  invalidated_time "
                            "from member limit " +
                            std::to_string(limit))) {

    members.push_back({
        // clang-format off
        .firstname              = r.get(0),
        .lastname               = r.get(1),
        .email                  = r.get(2),
        .dob                    = r.get<std::optional<date::sys_days>>(3),
        .date_of_last_logon     = r.get<std::optional<date::sys_seconds>>(4),
        .created_at             = r.get<std::optional<date::sys_seconds>>(5),
        .updated_at             = r.get<std::optional<date::sys_seconds>>(6),
        .email_failure_count    = r.get<int>(7),
        .invalid                = r.get<bool>(8),
        .invalidated_time       = r.get<std::optional<date::sys_seconds>>(9)
        // clang-format on
    });
  }
  return members;
}

struct tax_rate {
  int                              id;
  double                           rate;
  std::string                      description;
  std::optional<date::sys_seconds> date_last_modified;
  std::optional<date::sys_seconds> date_added;

  friend std::ostream& operator<<(std::ostream& os, const tax_rate& tr) {
    using mypp::format_time_point;

    os << tr.id << ": " << tr.description << ": " << tr.rate << "%\n";
    if (tr.date_last_modified)
      os << "date_last_modified=" << format_time_point(tr.date_last_modified.value()) << "\n";
    if (tr.date_added) os << "date_added=" << format_time_point(tr.date_added.value()) << "\n";
    return os;
  }
};

std::vector<tax_rate> load_tax_rates() {
  std::vector<tax_rate> tax_rates;

  for (auto&& r: db().query("select "
                            "  id, "
                            "  rate, "
                            "  description, "
                            "  date_last_modified, "
                            "  date_added "
                            "from tax_rate")) {

    tax_rates.push_back({
        // clang-format off
        .id                     = r.get<int>(0),
        .rate                   = r.get<double>(1),
        .description            = r.get(2),
        .date_last_modified     = r.get<std::optional<date::sys_days>>(3),
        .date_added             = r.get<std::optional<date::sys_seconds>>(4),
        // clang-format on
    });
  }
  return tax_rates;
}

struct order_lineitem {
  int         id;
  int         order_id;
  std::string description;
  double      tax_rate;
  double      value_without_tax;
  double      value_with_tax;

  friend std::ostream& operator<<(std::ostream& os, const order_lineitem& oli) {
    using mypp::format_time_point;

    // much faster than iostream (uses dragonbox for the floats?)
    os << fmt::format("{:d}: {:d}: {:s}: {:.4f}% {:.2f} {:.2f}", oli.id, oli.order_id,
                      oli.description, oli.tax_rate, oli.value_without_tax, oli.value_with_tax);

    return os;
  }
};

std::vector<order_lineitem> load_order_lineitems() {
  std::vector<order_lineitem> order_lineitems;

  for (auto&& r: db().query("select "
                            "  id, "
                            "  order_id, "
                            "  description, "
                            "  tax_rate, "
                            "  value_without_tax, "
                            "  value_with_tax "
                            "from order_lineitem")) {

    order_lineitems.push_back({
        // clang-format off
        .id                     = r.get<int>(0),
        .order_id               = r.get<int>(1),
        .description            = r.get(2),
        .tax_rate               = r.get<double>(3),
        .value_without_tax      = r.get<double>(4),
        .value_with_tax         = r.get<double>(5)
        // clang-format on
    });
  }
  return order_lineitems;
}

void avg_orders() {
  std::vector<order_lineitem> order_lineitems;

  std::vector<int> order_ids;
  long double      grand_total{};
  for (auto&& r: db().query("select "
                            "  order_id, "
                            "  value_with_tax "
                            "from order_lineitem")) {

    order_ids.push_back(r.get<int>(0));
    grand_total += r.get<double>(1);
  }
  std::sort(order_ids.begin(), order_ids.end());
  auto size = std::unique(order_ids.begin(), order_ids.end()) - order_ids.begin();

  std::cerr << fmt::format("orders {:22Ld}\n"
                           "total   {:22.6Lf}\n"
                           "average {:22.2Lf}\n",
                           order_ids.size(), grand_total, grand_total / size);
}

void report_topn(std::size_t N, const std::unordered_map<std::string, std::size_t>& map) {
  using MapType  = std::remove_cvref_t<decltype(map)>;
  using PairType = std::pair<MapType::key_type, MapType::mapped_type>;

  std::vector<PairType> topN(N);
  std::partial_sort_copy(map.begin(), map.end(), topN.begin(), topN.end(),
                         [](const auto& a, const auto& b) { return a.second > b.second; });

  for (auto&& p: topN) std::cout << fmt::format("{:20s} {:6d}", p.first, p.second) << "\n";

  std::cout << fmt::format(
      "{:20s} {:6d}\n", "Total",
      std::accumulate(map.begin(), map.end(), 0UL,
                      [](std::size_t count, const auto& freq) { return count += freq.second; }));
}

void report_stats(std::size_t N, const std::vector<member>& members) {
  std::unordered_map<std::string, std::size_t> invalid_freqs;
  std::unordered_map<std::string, std::size_t> failure_freqs;
  for (auto&& m: members) {
    std::string domain = m.email.substr(m.email.find('@') + 1);
    if (domain != "temp.webcollect.org.uk") {
      failure_freqs[domain] += static_cast<std::size_t>(m.email_failure_count);
      if (m.invalid) ++invalid_freqs[domain];
    }
  }

  std::cout << "invalid\n";
  report_topn(N, invalid_freqs);

  std::cout << "\nfailures\n";
  report_topn(N, failure_freqs);
}

int main(int argc, char* argv[]) {
  std::ios::sync_with_stdio(false);
  std::locale::global(std::locale(""));

  try {
    conf::init(std::filesystem::path(argv[0]).parent_path().append("myslice_demo.ini"));

    auto limit = argc > 1 ? std::stoi(argv[1]) : 10'000;
    auto N     = argc > 2 ? std::stoull(argv[2]) : 10;

    std::vector<member> members;
    {
      os::bch::Timer t("load members");
      members = load_members(limit);
    }
    {
      os::bch::Timer t("stats on members");
      report_stats(N, members);
    }
    {
      os::bch::Timer t("dump members");
      for (auto&& m: members) std::cout << m << "\n";
    }

    std::vector<tax_rate> tax_rates;
    {
      os::bch::Timer t("load/dump tax_rates");
      tax_rates = load_tax_rates();
      for (auto&& m: tax_rates) std::cout << m << "\n";
    }
    {
      os::bch::Timer t("fast avg olis");
      avg_orders();
    }
    std::vector<order_lineitem> olis;
    {
      os::bch::Timer t("load order_lineitems");
      olis = load_order_lineitems();
    }
    {
      os::bch::Timer t("dump olis");
      for (auto&& oli: olis) std::cout << oli << "\n";
    }
  } catch (const std::exception& e) {
    std::cerr << "Something went wrong. Exception thrown: " << e.what() << std::endl;
  }
}
