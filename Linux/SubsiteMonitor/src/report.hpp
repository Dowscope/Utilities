#pragma once

#include "statistics.hpp"

#include <iosfwd>
#include <string>

namespace subsite_monitor {

[[nodiscard]] std::string format_timestamp_utc(
    std::chrono::sys_seconds timestamp);

void print_statistics(const StatisticsReport& report, std::ostream& output);

} // namespace subsite_monitor
