#include "report.hpp"
#include "statistics.hpp"

#include <cstdlib>
#include <ctime>
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
