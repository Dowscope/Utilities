#pragma once

#include "log_parser.hpp"

#include <cstdint>
#include <filesystem>
#include <functional>

namespace subsite_monitor {

struct CollectionSummary {
    std::uint64_t parsed_records{};
    std::uint64_t malformed_lines{};
};

// Streams a log using a fixed-size line buffer. Throws std::runtime_error when
// the file cannot be opened or read.
[[nodiscard]] CollectionSummary collect_log(
    const std::filesystem::path& path);

// Streams a log and calls the handler once for each parsed record.
[[nodiscard]] CollectionSummary scan_log(
    const std::filesystem::path& path,
    const std::function<void(const AccessLogEntry&)>& on_record);

} // namespace subsite_monitor
