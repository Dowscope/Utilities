#pragma once

#include <chrono>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>

namespace subsite_monitor {

struct AccessLogEntry {
    std::string timestamp;
    std::string hostname;
    std::string client_ip;
    std::string method;
    std::string target;
    std::chrono::sys_seconds timestamp_utc{};
    std::uint16_t status{};
};

// Parses the documented subsite_monitor Nginx format. Returns no entry for
// empty, malformed, oversized-field, or invalid records.
[[nodiscard]] std::optional<AccessLogEntry> parse_access_log_line(
    std::string_view line);

} // namespace subsite_monitor
