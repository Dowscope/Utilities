#include "report.hpp"

#include <algorithm>
#include <ctime>
#include <iomanip>
#include <ostream>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

namespace subsite_monitor {

std::string format_timestamp_utc(std::chrono::sys_seconds timestamp) {
    using namespace std::chrono;
    const sys_days day_point = floor<days>(timestamp);
    const year_month_day date{day_point};
    const hh_mm_ss time{timestamp - day_point};

    std::ostringstream output;
    output << std::setfill('0') << std::setw(4)
           << static_cast<int>(date.year()) << '-' << std::setw(2)
           << static_cast<unsigned>(date.month()) << '-' << std::setw(2)
           << static_cast<unsigned>(date.day()) << 'T' << std::setw(2)
           << time.hours().count() << ':' << std::setw(2)
           << time.minutes().count() << ':' << std::setw(2)
           << time.seconds().count() << 'Z';
    return output.str();
}

std::string format_timestamp_local(std::chrono::sys_seconds timestamp) {
    const auto clock_time = std::chrono::time_point_cast<
        std::chrono::system_clock::duration>(timestamp);
    const std::time_t raw_time = std::chrono::system_clock::to_time_t(clock_time);
    std::tm local_time{};
    if (localtime_r(&raw_time, &local_time) == nullptr) {
        return "unavailable";
    }
    char buffer[40]{};
    const std::size_t length =
        std::strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S %z",
                      &local_time);
    if (length == 0U) {
        return "unavailable";
    }
    return std::string{buffer, length};
}

void print_statistics(const StatisticsReport& report, std::ostream& output) {
    std::vector<std::pair<std::string_view, const SiteStatistics*>> sites;
    sites.reserve(report.sites.size());
    std::size_t hostname_width = std::string_view{"Website"}.size();
    for (const auto& [hostname, statistics] : report.sites) {
        hostname_width = std::max(hostname_width, hostname.size());
        sites.emplace_back(hostname, &statistics);
    }
    std::sort(sites.begin(), sites.end(), [](const auto& left, const auto& right) {
        if (left.second->requests != right.second->requests) {
            return left.second->requests > right.second->requests;
        }
        return left.first < right.first;
    });

    output << std::left << std::setw(static_cast<int>(hostname_width))
           << "Website" << "  " << std::right << std::setw(8) << "Requests"
           << "  Last access (server time)\n";
    output << std::string(hostname_width + 31, '-') << '\n';
    for (const auto& [hostname, statistics] : sites) {
        output << std::left << std::setw(static_cast<int>(hostname_width))
               << hostname << "  " << std::right << std::setw(8)
               << statistics->requests << "  "
               << format_timestamp_local(statistics->latest_access_utc) << '\n';
    }

    output << "\nDaily requests by website (date from access log)\n"
           << std::left << std::setw(10) << "Date" << "  "
           << std::setw(static_cast<int>(hostname_width)) << "Website" << "  "
           << std::right << "Requests\n";
    output << std::string(hostname_width + 29, '-') << '\n';
    for (const auto& [hostname, statistics] : report.sites) {
        for (const auto& [date, requests] : statistics.requests_by_day) {
            output << std::left << std::setw(10) << date << "  "
                   << std::setw(static_cast<int>(hostname_width)) << hostname
                   << "  " << std::right << requests << '\n';
        }
    }
    output << "\nMalformed lines: " << report.malformed_lines << '\n';
}

} // namespace subsite_monitor
