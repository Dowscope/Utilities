#include "statistics.hpp"

#include "collector.hpp"
#include "log_parser.hpp"

#include <stdexcept>

namespace subsite_monitor {
namespace {

constexpr std::size_t max_sites = 10000;
constexpr std::size_t max_site_day_buckets = 100000;

} // namespace

StatisticsReport build_statistics(const std::filesystem::path& path) {
    StatisticsReport report;
    std::size_t daily_bucket_count = 0;
    const CollectionSummary summary = scan_log(
        path, [&report, &daily_bucket_count](const AccessLogEntry& entry) {
            auto site_position = report.sites.find(entry.hostname);
            if (site_position == report.sites.end()) {
                if (report.sites.size() >= max_sites) {
                    throw std::runtime_error(
                        "statistics limit exceeded: too many distinct websites");
                }
                site_position =
                    report.sites.emplace(entry.hostname, SiteStatistics{}).first;
            }
            SiteStatistics& site = site_position->second;
            ++site.requests;
            if (site.requests == 1 ||
                entry.timestamp_utc > site.latest_access_utc) {
                site.latest_access_utc = entry.timestamp_utc;
            }

            const std::string date = entry.timestamp.substr(0, 10);
            auto day_position = site.requests_by_day.find(date);
            if (day_position == site.requests_by_day.end()) {
                if (daily_bucket_count >= max_site_day_buckets) {
                    throw std::runtime_error(
                        "statistics limit exceeded: too many website/day pairs");
                }
                day_position = site.requests_by_day.emplace(date, 0).first;
                ++daily_bucket_count;
            }
            ++day_position->second;
        });
    report.malformed_lines = summary.malformed_lines;
    return report;
}

} // namespace subsite_monitor
