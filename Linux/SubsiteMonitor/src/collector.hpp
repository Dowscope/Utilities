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

struct LogBatch {
    CollectionSummary summary;
    std::uint64_t next_offset{};
    bool discard_until_newline{};
};

// Streams a log using a fixed-size line buffer. Throws std::runtime_error when
// the file cannot be opened or read.
[[nodiscard]] CollectionSummary collect_log(
    const std::filesystem::path& path);

// Streams a log and calls the handler once for each parsed record.
[[nodiscard]] CollectionSummary scan_log(
    const std::filesystem::path& path,
    const std::function<void(const AccessLogEntry&)>& on_record);

// Reads complete lines from a file descriptor beginning at `offset`. An
// incomplete final line is left for the next poll. `max_bytes` bounds each
// batch, except that one overlong line is consumed through its newline.
[[nodiscard]] LogBatch scan_log_batch(
    int descriptor, std::uint64_t offset, std::size_t max_bytes,
    bool discard_until_newline,
    const std::function<void(const AccessLogEntry&)>& on_record);

} // namespace subsite_monitor
