#pragma once

#include "log_parser.hpp"

#include <chrono>
#include <cstdint>
#include <filesystem>
#include <functional>
#include <map>
#include <string>

namespace subsite_monitor {

struct SiteStatistics {
    std::uint64_t requests{};
    std::chrono::sys_seconds latest_access_utc{};
    std::map<std::string, std::uint64_t> requests_by_day;
    std::map<std::string, std::chrono::sys_seconds> latest_access_by_day;
};

struct StatisticsReport {
    std::map<std::string, SiteStatistics> sites;
    std::uint64_t malformed_lines{};
};

// Calculates per-site and per-log-date statistics from one access log.
[[nodiscard]] StatisticsReport build_statistics(
    const std::filesystem::path& path,
    const std::function<void(const AccessLogEntry&)>& on_record = {});

} // namespace subsite_monitor
