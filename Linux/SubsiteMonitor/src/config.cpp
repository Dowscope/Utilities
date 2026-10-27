#include "config.hpp"

#include <array>
#include <cerrno>
#include <cstddef>
#include <fstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <system_error>

#include <sys/stat.h>
#include <unistd.h>

namespace subsite_monitor {
namespace {

constexpr std::size_t max_config_size = 16U * 1024U;
constexpr std::size_t max_line_size = 4096U;

std::runtime_error filesystem_error(std::string_view action,
                                    const std::filesystem::path& path,
                                    int error_number = errno) {
    return std::runtime_error(std::string{action} + " '" + path.string() +
                              "': " +
                              std::system_category().message(error_number));
}

void require_secure_file(const std::filesystem::path& path) {
    struct stat file_status {};
    if (lstat(path.c_str(), &file_status) != 0) {
        throw filesystem_error("cannot inspect configuration", path);
    }
    if (!S_ISREG(file_status.st_mode)) {
        throw std::runtime_error("configuration must be a regular file");
    }
    if (file_status.st_uid != 0) {
        throw std::runtime_error("configuration must be owned by root");
    }
    if ((file_status.st_mode & 0077) != 0 ||
        (file_status.st_mode & S_IRUSR) == 0) {
        throw std::runtime_error(
            "configuration must be readable by root only (mode 0600)");
    }
    if (file_status.st_size < 0 ||
        static_cast<unsigned long long>(file_status.st_size) >
            static_cast<unsigned long long>(max_config_size)) {
        throw std::runtime_error("configuration file is too large");
    }

    const std::filesystem::path parent = path.parent_path().empty()
                                             ? std::filesystem::path{"."}
                                             : path.parent_path();
    struct stat parent_status {};
    if (stat(parent.c_str(), &parent_status) != 0) {
        throw filesystem_error("cannot inspect configuration directory", parent);
    }
    if (!S_ISDIR(parent_status.st_mode) || parent_status.st_uid != 0 ||
        (parent_status.st_mode & 0022) != 0) {
        throw std::runtime_error(
            "configuration directory must be root-owned and not group/world writable");
    }
}

std::string_view trim(std::string_view value) {
    while (!value.empty() &&
           (value.front() == ' ' || value.front() == '\t' ||
            value.front() == '\r')) {
        value.remove_prefix(1);
    }
    while (!value.empty() &&
           (value.back() == ' ' || value.back() == '\t' ||
            value.back() == '\r')) {
        value.remove_suffix(1);
    }
    return value;
}

} // namespace

MonitorConfig parse_config_text(std::string_view text) {
    if (text.size() > max_config_size) {
        throw std::runtime_error("configuration file is too large");
    }
    MonitorConfig config;
    bool access_log_seen = false;
    std::size_t line_start = 0;
    while (line_start < text.size()) {
        const std::size_t line_end = text.find('\n', line_start);
        const std::size_t bounded_line_end =
            line_end == std::string_view::npos ? text.size() : line_end;
        std::string_view line =
            text.substr(line_start, bounded_line_end - line_start);
        if (line.size() > max_line_size) {
            throw std::runtime_error("configuration contains an overlong line");
        }
        line = trim(line);
        if (!line.empty() && line.front() != '#') {
            const std::size_t separator = line.find('=');
            if (separator == std::string_view::npos) {
                throw std::runtime_error(
                    "configuration entry must use key = value");
            }
            const std::string_view key = trim(line.substr(0, separator));
            const std::string_view value = trim(line.substr(separator + 1U));
            if (key != "access_log") {
                throw std::runtime_error("unknown configuration key: " +
                                         std::string{key});
            }
            if (access_log_seen) {
                throw std::runtime_error("configuration repeats access_log");
            }
            if (value.empty() || value.size() > 4096U) {
                throw std::runtime_error(
                    "access_log must be a non-empty path under 4096 bytes");
            }
            if (value.front() != '/') {
                throw std::runtime_error("access_log must be an absolute path");
            }
            for (const char character : value) {
                const auto byte = static_cast<unsigned char>(character);
                if (byte < 0x20U || byte == 0x7fU) {
                    throw std::runtime_error(
                        "access_log path contains a control character");
                }
            }
            config.access_log = value;
            access_log_seen = true;
        }
        if (line_end == std::string_view::npos) {
            break;
        }
        line_start = line_end + 1U;
    }
    if (!access_log_seen) {
        throw std::runtime_error("configuration is missing access_log");
    }
    return config;
}

MonitorConfig load_config(const std::filesystem::path& path) {
    if (geteuid() != 0) {
        throw std::runtime_error(
            "the default configuration is root-only; run with sudo or pass --log");
    }
    require_secure_file(path);

    std::ifstream input(path, std::ios::binary);
    if (!input) {
        throw filesystem_error("cannot read configuration", path);
    }
    std::array<char, max_config_size + 1U> buffer{};
    input.read(buffer.data(), static_cast<std::streamsize>(buffer.size()));
    const std::streamsize extracted = input.gcount();
    if (extracted < 0 ||
        static_cast<unsigned long long>(extracted) >
            static_cast<unsigned long long>(max_config_size)) {
        throw std::runtime_error("configuration file is too large");
    }
    if (input.bad()) {
        throw filesystem_error("cannot read configuration", path, EIO);
    }
    return parse_config_text(
        std::string_view{buffer.data(), static_cast<std::size_t>(extracted)});
}

} // namespace subsite_monitor
