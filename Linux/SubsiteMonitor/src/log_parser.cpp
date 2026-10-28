#include "log_parser.hpp"

#include <chrono>
#include <limits>
#include <string_view>

namespace subsite_monitor {
namespace {

constexpr std::size_t max_timestamp_length = 32;
constexpr std::size_t max_hostname_length = 253;
constexpr std::size_t max_client_ip_length = 64;
constexpr std::size_t max_method_length = 32;
constexpr std::size_t max_target_length = 8192;

bool is_digit(char value) {
    return value >= '0' && value <= '9';
}

bool is_hex_digit(char value) {
    return is_digit(value) || (value >= 'a' && value <= 'f') ||
           (value >= 'A' && value <= 'F');
}

unsigned hex_value(char value) {
    if (is_digit(value)) {
        return static_cast<unsigned>(value - '0');
    }
    if (value >= 'a' && value <= 'f') {
        return static_cast<unsigned>(value - 'a' + 10);
    }
    return static_cast<unsigned>(value - 'A' + 10);
}

bool parse_two_digits(std::string_view value, std::size_t offset,
                      unsigned& result) {
    if (offset + 1 >= value.size() || !is_digit(value[offset]) ||
        !is_digit(value[offset + 1])) {
        return false;
    }
    result = static_cast<unsigned>(value[offset] - '0') * 10U +
             static_cast<unsigned>(value[offset + 1] - '0');
    return true;
}

bool parse_timestamp(std::string_view value,
                     std::chrono::sys_seconds& timestamp_utc) {
    if (value.size() != 25 || value[4] != '-' || value[7] != '-' ||
        value[10] != 'T' || value[13] != ':' || value[16] != ':' ||
        (value[19] != '+' && value[19] != '-') || value[22] != ':') {
        return false;
    }

    for (const std::size_t offset : {0U, 1U, 2U, 3U, 5U, 6U, 8U, 9U,
                                     11U, 12U, 14U, 15U, 17U, 18U, 20U,
                                     21U, 23U, 24U}) {
        if (!is_digit(value[offset])) {
            return false;
        }
    }

    const int year = (value[0] - '0') * 1000 + (value[1] - '0') * 100 +
                     (value[2] - '0') * 10 + (value[3] - '0');
    unsigned month = 0;
    unsigned day = 0;
    unsigned hour = 0;
    unsigned minute = 0;
    unsigned second = 0;
    unsigned offset_hour = 0;
    unsigned offset_minute = 0;
    if (!parse_two_digits(value, 5, month) ||
        !parse_two_digits(value, 8, day) ||
        !parse_two_digits(value, 11, hour) ||
        !parse_two_digits(value, 14, minute) ||
        !parse_two_digits(value, 17, second) ||
        !parse_two_digits(value, 20, offset_hour) ||
        !parse_two_digits(value, 23, offset_minute)) {
        return false;
    }

    const std::chrono::year_month_day date{
        std::chrono::year{year}, std::chrono::month{month},
        std::chrono::day{day}};
    if (!date.ok() || hour > 23 || minute > 59 || second > 60 ||
        offset_hour > 23 || offset_minute > 59) {
        return false;
    }

    auto local_time = std::chrono::sys_days{date} +
                      std::chrono::hours{hour} +
                      std::chrono::minutes{minute} +
                      std::chrono::seconds{second};
    auto offset = std::chrono::hours{offset_hour} +
                  std::chrono::minutes{offset_minute};
    if (value[19] == '-') {
        offset = -offset;
    }
    timestamp_utc = std::chrono::time_point_cast<std::chrono::seconds>(
        local_time - offset);
    return true;
}

bool valid_hostname(std::string_view value) {
    if (value.empty() || value.size() > max_hostname_length ||
        value.front() == '.' || value.back() == '.') {
        return false;
    }
    for (const char character : value) {
        const bool allowed = (character >= 'a' && character <= 'z') ||
                             (character >= 'A' && character <= 'Z') ||
                             is_digit(character) || character == '.' ||
                             character == '-' || character == '_';
        if (!allowed) {
            return false;
        }
    }
    return true;
}

bool valid_client_ip(std::string_view value) {
    if (value.empty() || value.size() > max_client_ip_length) {
        return false;
    }
    for (const char character : value) {
        const bool allowed = (character >= 'a' && character <= 'z') ||
                             (character >= 'A' && character <= 'Z') ||
                             is_digit(character) || character == '.' ||
                             character == ':' || character == '%' ||
                             character == '-' || character == '_';
        if (!allowed) {
            return false;
        }
    }
    return true;
}

bool valid_method(std::string_view value) {
    if (value.empty() || value.size() > max_method_length) {
        return false;
    }
    constexpr std::string_view punctuation = "!#$%&'*+-.^_`|~";
    for (const char character : value) {
        const bool alpha = (character >= 'a' && character <= 'z') ||
                           (character >= 'A' && character <= 'Z');
        if (!alpha && !is_digit(character) &&
            punctuation.find(character) == std::string_view::npos) {
            return false;
        }
    }
    return true;
}

bool valid_target(std::string_view value) {
    if (value.empty() || value.size() > max_target_length) {
        return false;
    }
    for (const unsigned char character : value) {
        if (character <= 0x20U || character == 0x7fU) {
            return false;
        }
    }
    return true;
}

bool next_token(std::string_view input, std::size_t& position,
                std::string_view& token) {
    while (position < input.size() && input[position] == ' ') {
        ++position;
    }
    const std::size_t start = position;
    while (position < input.size() && input[position] != ' ') {
        ++position;
    }
    if (start == position) {
        return false;
    }
    token = input.substr(start, position - start);
    return true;
}

bool parse_request(std::string_view request, std::string_view& method,
                   std::string_view& target) {
    const std::size_t first_space = request.find(' ');
    if (first_space == std::string_view::npos) {
        return false;
    }
    const std::size_t second_space = request.find(' ', first_space + 1);
    if (second_space == std::string_view::npos ||
        request.find(' ', second_space + 1) != std::string_view::npos) {
        return false;
    }

    method = request.substr(0, first_space);
    target = request.substr(first_space + 1, second_space - first_space - 1);
    const std::string_view version = request.substr(second_space + 1);
    return valid_method(method) && valid_target(target) &&
           (version == "HTTP/1.0" || version == "HTTP/1.1" ||
            version == "HTTP/2.0" || version == "HTTP/3.0");
}

} // namespace

std::optional<AccessLogEntry> parse_access_log_line(std::string_view line) {
    if (line.empty() || line.size() > 16384) {
        return std::nullopt;
    }
    if (!line.empty() && line.back() == '\r') {
        line.remove_suffix(1);
    }

    std::size_t position = 0;
    std::string_view timestamp;
    std::string_view hostname;
    std::string_view client_ip;
    std::chrono::sys_seconds timestamp_utc{};
    if (!next_token(line, position, timestamp) ||
        !next_token(line, position, hostname) ||
        !next_token(line, position, client_ip) ||
        !parse_timestamp(timestamp, timestamp_utc) ||
        timestamp.size() > max_timestamp_length ||
        !valid_hostname(hostname) || !valid_client_ip(client_ip)) {
        return std::nullopt;
    }

    while (position < line.size() && line[position] == ' ') {
        ++position;
    }
    if (position >= line.size() || line[position] != '"') {
        return std::nullopt;
    }
    ++position;

    std::string request;
    request.reserve(128);
    bool closed_quote = false;
    while (position < line.size()) {
        const char character = line[position++];
        if (character == '"') {
            closed_quote = true;
            break;
        }
        if (character == '\\') {
            if (position >= line.size()) {
                return std::nullopt;
            }
            const char escaped = line[position++];
            if (escaped == '"' || escaped == '\\') {
                request.push_back(escaped);
            } else if (escaped == 'x' && position + 1 < line.size() &&
                       is_hex_digit(line[position]) &&
                       is_hex_digit(line[position + 1])) {
                const unsigned decoded = hex_value(line[position]) * 16U +
                                         hex_value(line[position + 1]);
                request.push_back(static_cast<char>(decoded));
                position += 2;
            } else {
                return std::nullopt;
            }
        } else {
            request.push_back(character);
        }
        if (request.size() > max_method_length + max_target_length + 16U) {
            return std::nullopt;
        }
    }
    if (!closed_quote) {
        return std::nullopt;
    }

    std::string_view method;
    std::string_view target;
    if (!parse_request(request, method, target)) {
        return std::nullopt;
    }

    std::string_view status_text;
    if (!next_token(line, position, status_text) || status_text.size() != 3) {
        return std::nullopt;
    }
    while (position < line.size() && line[position] == ' ') {
        ++position;
    }
    if (position != line.size()) {
        return std::nullopt;
    }

    unsigned status = 0;
    for (const char digit : status_text) {
        if (!is_digit(digit)) {
            return std::nullopt;
        }
        status = status * 10U + static_cast<unsigned>(digit - '0');
    }
    if (status < 100U || status > 599U ||
        status > std::numeric_limits<std::uint16_t>::max()) {
        return std::nullopt;
    }

    return AccessLogEntry{std::string{timestamp}, std::string{hostname},
                          std::string{client_ip}, std::string{method},
                          std::string{target}, timestamp_utc,
                          static_cast<std::uint16_t>(status)};
}

std::optional<std::chrono::sys_seconds> parse_utc_timestamp(
    std::string_view value) {
    if (value.size() != 20 || value.back() != 'Z') {
        return std::nullopt;
    }
    std::string timestamp{value.substr(0, 19)};
    timestamp += "+00:00";
    std::chrono::sys_seconds result{};
    if (!parse_timestamp(timestamp, result)) {
        return std::nullopt;
    }
    return result;
}

} // namespace subsite_monitor
