#include "report.hpp"
#include "statistics.hpp"

#include <cstdlib>
#include <ctime>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string_view>

namespace {

void require(bool condition, std::string_view message) {
    if (!condition) {
        std::cerr << "Statistics test failed: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

} // namespace

int main() {
    (void)setenv("TZ", "UTC0", 1);
    tzset();
    const subsite_monitor::StatisticsReport report =
        subsite_monitor::build_statistics("tests/fixtures/access.log");

    require(report.sites.size() == 2, "aggregate requests by virtual host");
    require(report.malformed_lines == 2, "count malformed log lines");

    using namespace std::chrono;
    using namespace subsite_monitor;
    const sys_days today{2026y / October / 27};
    const sys_seconds now = today + hours{12};
    StatisticsReport activity_report;
    SiteStatistics activity_site;
    const auto add_day = [&activity_site](sys_days day, std::uint64_t count) {
        const year_month_day date{day};
        std::ostringstream value;
        value << std::setfill('0') << std::setw(4)
              << static_cast<int>(date.year()) << '-' << std::setw(2)
              << static_cast<unsigned>(date.month()) << '-' << std::setw(2)
              << static_cast<unsigned>(date.day());
        activity_site.requests_by_day.emplace(value.str(), count);
    };
    add_day(sys_days{2025y / October / 26}, 100);
    add_day(sys_days{2025y / October / 27}, 1);
    add_day(sys_days{2026y / April / 27}, 2);
    add_day(today - days{89}, 3);
    add_day(today - days{29}, 4);
    add_day(today - days{6}, 5);
    add_day(today - days{1}, 6);
    add_day(today, 7);
    activity_report.sites.emplace("activity.test", activity_site);
    SiteStatistics other_activity_site;
    other_activity_site.requests = 9;
    other_activity_site.requests_by_day.emplace("2026-10-27", 9);
    activity_report.sites.emplace("other.test", other_activity_site);
    const std::map<std::string, std::uint64_t> recent_requests{
        {"activity.test", 8}, {"other.test", 2}};
    const RequestActivityTotals activity = calculate_request_activity_totals(
        activity_report, now, recent_requests);
    const RequestPeriodCounts activity_site_counts =
        activity.by_site.at("activity.test");
    require(activity_site_counts.one_year == 28,
            "sum the site's trailing calendar year");
    require(activity_site_counts.six_months == 27,
            "sum the site's trailing calendar six months");
    require(activity_site_counts.ninety_days == 25,
            "sum the site's trailing 90 calendar days");
    require(activity_site_counts.thirty_days == 22,
            "sum the site's trailing 30 calendar days");
    require(activity_site_counts.seven_days == 18,
            "sum the site's trailing seven calendar days");
    require(activity_site_counts.twenty_four_hours == 8,
            "preserve the exact rolling 24-hour count");
    require(activity_site_counts.today == 7,
            "sum this site's requests since local midnight");
    require(activity.by_site.at("other.test").today == 9 &&
                activity.all_sites.today == 16 &&
                activity.all_sites.twenty_four_hours == 10,
            "keep request windows independent for each website");

    const auto live = report.sites.find("live.lakeheadschools.ca");
    require(live != report.sites.end(), "include live virtual host");
    require(live->second.requests == 4, "count all requests for live host");
    require(live->second.requests_by_day.at("2026-10-26") == 1,
            "count requests for the first log date");
    require(live->second.requests_by_day.at("2026-10-27") == 3,
            "count requests for the second log date");

    const auto test = report.sites.find("test.lakeheadschools.ca");
    require(test != report.sites.end() && test->second.requests == 1,
            "count requests for the second virtual host");

    subsite_monitor::StatisticsReport sort_report = report;
    sort_report.sites.emplace(
        "a-first.test", subsite_monitor::SiteStatistics{
                            1, live->second.latest_access_utc, {}, {}});
    sort_report.sites.emplace(
        "z-last.test", subsite_monitor::SiteStatistics{
                            2, live->second.latest_access_utc, {}, {}});

    std::ostringstream output;
    subsite_monitor::print_statistics(sort_report, output);
    const std::string text = output.str();
    const std::size_t higher_count_row = text.find("z-last.test");
    const std::size_t lower_count_row = text.find("a-first.test");
    require(higher_count_row != std::string::npos &&
                lower_count_row != std::string::npos &&
                higher_count_row < lower_count_row,
            "sort the site table by request count descending");
    require(text.find("2026-10-27 14:45:32 +0000") != std::string::npos,
            "display latest access in the server's configured local timezone");
    require(text.find("203.0.113.") == std::string::npos &&
                text.find("token=hidden") == std::string::npos,
            "omit client IPs and request targets from the report");
    require(text.find("Daily requests by website") != std::string::npos,
            "include the daily request section");

    std::cout << "Statistics tests passed\n";
}
