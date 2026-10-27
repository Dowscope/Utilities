#include "collector.hpp"

#include "log_parser.hpp"

#include <array>
#include <fstream>
#include <stdexcept>
#include <string_view>

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

} // namespace subsite_monitor
