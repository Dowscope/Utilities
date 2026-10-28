#include "collector.hpp"

#include "log_parser.hpp"

#include <array>
#include <cerrno>
#include <fstream>
#include <limits>
#include <stdexcept>
#include <string_view>

#include <sys/types.h>
#include <unistd.h>

namespace subsite_monitor {
namespace {

constexpr std::size_t max_line_length = 16384;

enum class LineResult { line, too_long, end };

LineResult read_bounded_line(std::istream& input,
                             std::array<char, max_line_length>& buffer,
                             std::size_t& length) {
    length = 0;
    bool received_character = false;
    bool too_long = false;
    char character = '\0';
    while (input.get(character)) {
        if (character == '\n') {
            return too_long ? LineResult::too_long : LineResult::line;
        }
        received_character = true;
        if (length < buffer.size()) {
            buffer[length++] = character;
        } else {
            too_long = true;
        }
    }

    if (input.bad()) {
        throw std::runtime_error("error while reading access log");
    }
    if (!received_character) {
        return LineResult::end;
    }
    return too_long ? LineResult::too_long : LineResult::line;
}

} // namespace

CollectionSummary collect_log(const std::filesystem::path& path) {
    return scan_log(path, {});
}

CollectionSummary scan_log(
    const std::filesystem::path& path,
    const std::function<void(const AccessLogEntry&)>& on_record) {
    std::ifstream input{path};
    if (!input) {
        throw std::runtime_error("cannot open access log: " + path.string());
    }

    CollectionSummary summary;
    std::array<char, max_line_length> buffer{};
    std::size_t length = 0;
    while (true) {
        const LineResult result = read_bounded_line(input, buffer, length);
        if (result == LineResult::end) {
            break;
        }
        if (result == LineResult::too_long) {
            ++summary.malformed_lines;
        } else {
            const auto entry =
                parse_access_log_line(std::string_view{buffer.data(), length});
            if (!entry) {
                ++summary.malformed_lines;
            } else {
                ++summary.parsed_records;
                if (on_record) {
                    on_record(*entry);
                }
            }
        }
    }
    return summary;
}

LogBatch scan_log_batch(
    int descriptor, std::uint64_t offset, std::size_t max_bytes,
    bool discard_until_newline,
    const std::function<void(const AccessLogEntry&)>& on_record) {
    if (descriptor < 0 || max_bytes == 0U ||
        offset > static_cast<std::uint64_t>(
                     std::numeric_limits<off_t>::max())) {
        throw std::runtime_error("invalid access-log batch position");
    }

    LogBatch batch;
    batch.next_offset = offset;
    batch.discard_until_newline = discard_until_newline;
    std::array<char, max_line_length> line_buffer{};
    std::array<char, 16384> read_buffer{};
    std::size_t line_length = 0;
    bool line_too_long = false;
    std::uint64_t line_start = offset;
    std::uint64_t cursor = offset;
    bool finished_batch = false;

    while (!finished_batch) {
        if (cursor > static_cast<std::uint64_t>(
                         std::numeric_limits<off_t>::max())) {
            throw std::runtime_error("access-log offset exceeds file limits");
        }
        const ssize_t bytes_read = pread(
            descriptor, read_buffer.data(), read_buffer.size(),
            static_cast<off_t>(cursor));
        if (bytes_read < 0) {
            if (errno == EINTR) {
                continue;
            }
            throw std::runtime_error("cannot read access-log batch");
        }
        if (bytes_read == 0) {
            if (batch.discard_until_newline) {
                batch.next_offset = cursor;
            } else if (line_length > 0U || line_too_long) {
                batch.next_offset = line_start;
            } else {
                batch.next_offset = cursor;
            }
            break;
        }

        for (ssize_t index = 0; index < bytes_read; ++index) {
            const char character = read_buffer[static_cast<std::size_t>(index)];
            ++cursor;
            if (batch.discard_until_newline) {
                if (character == '\n') {
                    batch.discard_until_newline = false;
                    line_start = cursor;
                    if (cursor - offset >= max_bytes) {
                        batch.next_offset = cursor;
                        finished_batch = true;
                        break;
                    }
                }
                continue;
            }
            if (character != '\n') {
                if (line_length == 0U && !line_too_long) {
                    line_start = cursor - 1U;
                }
                if (line_length < line_buffer.size()) {
                    line_buffer[line_length++] = character;
                } else {
                    line_too_long = true;
                }
                continue;
            }

            if (line_too_long) {
                ++batch.summary.malformed_lines;
            } else {
                const auto entry = parse_access_log_line(
                    std::string_view{line_buffer.data(), line_length});
                if (!entry) {
                    ++batch.summary.malformed_lines;
                } else {
                    ++batch.summary.parsed_records;
                    if (on_record) {
                        on_record(*entry);
                    }
                }
            }
            line_length = 0;
            line_too_long = false;
            line_start = cursor;
            batch.next_offset = cursor;
            if (cursor - offset >= max_bytes) {
                finished_batch = true;
                break;
            }
        }
    }
    return batch;
}

} // namespace subsite_monitor
