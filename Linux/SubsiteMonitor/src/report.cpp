#include "report.hpp"

#include <algorithm>
#include <array>
#include <ctime>
#include <iomanip>
#include <limits>
#include <ostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace subsite_monitor {
namespace {

std::chrono::year_month_day local_date(std::chrono::sys_seconds timestamp) {
    const auto clock_time = std::chrono::time_point_cast<
        std::chrono::system_clock::duration>(timestamp);
    const std::time_t raw_time = std::chrono::system_clock::to_time_t(clock_time);
    std::tm local_time{};
    if (localtime_r(&raw_time, &local_time) == nullptr) {
        throw std::runtime_error("cannot determine the server's local date");
    }
    return {std::chrono::year{local_time.tm_year + 1900},
            std::chrono::month{static_cast<unsigned>(local_time.tm_mon + 1)},
            std::chrono::day{static_cast<unsigned>(local_time.tm_mday)}};
}

std::string date_string(std::chrono::year_month_day date) {
    std::ostringstream output;
    output << std::setfill('0') << std::setw(4)
           << static_cast<int>(date.year()) << '-' << std::setw(2)
           << static_cast<unsigned>(date.month()) << '-' << std::setw(2)
           << static_cast<unsigned>(date.day());
    return output.str();
}

std::chrono::year_month_day subtract_months(
    std::chrono::year_month_day date, unsigned month_count) {
    const std::chrono::year_month target_month =
        date.year() / date.month() - std::chrono::months{month_count};
    const std::chrono::year_month_day_last last_day{
        target_month.year(), std::chrono::month_day_last{target_month.month()}};
    const unsigned day = std::min(static_cast<unsigned>(date.day()),
                                  static_cast<unsigned>(last_day.day()));
    return {target_month.year(), target_month.month(),
            std::chrono::day{day}};
}

std::uint64_t sum_requests(const SiteStatistics& site,
                           std::string_view first_date,
                           std::string_view last_date) {
    std::uint64_t total = 0;
    auto day = site.requests_by_day.lower_bound(std::string{first_date});
    while (day != site.requests_by_day.end() && day->first <= last_date) {
        if (day->second > std::numeric_limits<std::uint64_t>::max() - total) {
            throw std::runtime_error("request activity total overflow");
        }
        total += day->second;
        ++day;
    }
    return total;
}

void add_count(std::uint64_t& total, std::uint64_t count) {
    if (count > std::numeric_limits<std::uint64_t>::max() - total) {
        throw std::runtime_error("request activity total overflow");
    }
    total += count;
}

void add_period_counts(RequestPeriodCounts& total,
                       const RequestPeriodCounts& site) {
    add_count(total.one_year, site.one_year);
    add_count(total.six_months, site.six_months);
    add_count(total.ninety_days, site.ninety_days);
    add_count(total.thirty_days, site.thirty_days);
    add_count(total.seven_days, site.seven_days);
    add_count(total.today, site.today);
    if (site.twenty_four_hours) {
        if (!total.twenty_four_hours) {
            total.twenty_four_hours = 0;
        }
        add_count(*total.twenty_four_hours, *site.twenty_four_hours);
    }
}

} // namespace

RequestActivityTotals calculate_request_activity_totals(
    const StatisticsReport& report, std::chrono::sys_seconds now,
    const std::optional<std::map<std::string, std::uint64_t>>&
        twenty_four_hour_requests) {
    using namespace std::chrono;
    const year_month_day today = local_date(now);
    const sys_days today_days{today};
    const std::string today_string = date_string(today);
    const auto count_from = [&today_string](const SiteStatistics& site,
                                            sys_days first_day) {
        return sum_requests(site, date_string(year_month_day{first_day}),
                            today_string);
    };

    RequestActivityTotals totals;
    for (const auto& [hostname, site] : report.sites) {
        RequestPeriodCounts counts;
        counts.one_year = sum_requests(
            site, date_string(subtract_months(today, 12)), today_string);
        counts.six_months = sum_requests(
            site, date_string(subtract_months(today, 6)), today_string);
        counts.ninety_days = count_from(site, today_days - days{89});
        counts.thirty_days = count_from(site, today_days - days{29});
        counts.seven_days = count_from(site, today_days - days{6});
        counts.today = sum_requests(site, today_string, today_string);
        if (twenty_four_hour_requests) {
            const auto recent = twenty_four_hour_requests->find(hostname);
            counts.twenty_four_hours =
                recent == twenty_four_hour_requests->end() ? 0 : recent->second;
        }
        add_period_counts(totals.all_sites, counts);
        totals.by_site.emplace(hostname, counts);
    }
    return totals;
}

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
