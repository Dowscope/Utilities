#pragma once

#include "statistics.hpp"

#include <cstdint>
#include <iosfwd>
#include <map>
#include <optional>
#include <string>

namespace subsite_monitor {

enum class RequestPeriod {
    today,
    one_year,
    six_months,
    ninety_days,
    thirty_days,
    seven_days,
    twenty_four_hours,
};

struct RequestPeriodCounts {
    std::uint64_t one_year{};
    std::uint64_t six_months{};
    std::uint64_t ninety_days{};
    std::uint64_t thirty_days{};
    std::uint64_t seven_days{};
    std::optional<std::uint64_t> twenty_four_hours;
    std::uint64_t today{};
};

struct RequestActivityTotals {
    RequestPeriodCounts all_sites;
    std::map<std::string, RequestPeriodCounts> by_site;
};

[[nodiscard]] RequestActivityTotals calculate_request_activity_totals(
    const StatisticsReport& report, std::chrono::sys_seconds now,
    const std::optional<std::map<std::string, std::uint64_t>>&
        twenty_four_hour_requests);

[[nodiscard]] std::string format_timestamp_utc(
    std::chrono::sys_seconds timestamp);
[[nodiscard]] std::string format_timestamp_local(
    std::chrono::sys_seconds timestamp);

void print_statistics(const StatisticsReport& report, std::ostream& output);

} // namespace subsite_monitor
