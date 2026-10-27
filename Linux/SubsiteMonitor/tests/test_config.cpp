#include "config.hpp"

#include <cassert>
#include <filesystem>
#include <fstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

#include <sys/stat.h>
#include <unistd.h>

namespace {

bool throws_with(std::string_view expected_text,
                 const std::filesystem::path& path) {
    try {
        (void)subsite_monitor::load_config(path);
    } catch (const std::exception& error) {
        return std::string_view{error.what()}.find(expected_text) !=
               std::string_view::npos;
    }
    return false;
}

bool parse_throws_with(std::string_view expected_text, std::string_view text) {
    try {
        (void)subsite_monitor::parse_config_text(text);
    } catch (const std::exception& error) {
        return std::string_view{error.what()}.find(expected_text) !=
               std::string_view::npos;
    }
    return false;
}

} // namespace

int main() {
    const subsite_monitor::MonitorConfig parsed =
        subsite_monitor::parse_config_text(
            "# default path\naccess_log = /var/log/nginx/access.log\n");
    assert(parsed.access_log == "/var/log/nginx/access.log");
    assert(parse_throws_with("unknown configuration key",
                             "unknown = value\n"));
    assert(parse_throws_with("repeats access_log",
                             "access_log = /tmp/a\naccess_log = /tmp/b\n"));
    assert(parse_throws_with("absolute path", "access_log = relative.log\n"));
    assert(parse_throws_with("missing access_log", "# comment only\n"));
    assert(parse_throws_with("control character",
                             "access_log = /tmp/bad\x01path\n"));

    std::string pattern = "/tmp/subsite-monitor-config-XXXXXX";
    std::vector<char> directory_name(pattern.begin(), pattern.end());
    directory_name.push_back('\0');
    char* created_directory = mkdtemp(directory_name.data());
    assert(created_directory != nullptr);
    const std::filesystem::path directory{created_directory};
    const std::filesystem::path config_path = directory / "monitor.conf";

    if (geteuid() != 0) {
        assert(throws_with("root-only", config_path));
        std::filesystem::remove_all(directory);
        return 0;
    }

    {
        std::ofstream output{config_path};
        output << "# test configuration\n"
                  "access_log = /var/log/nginx/access.log\n";
        assert(output.good());
    }
    assert(chmod(config_path.c_str(), S_IRUSR | S_IWUSR) == 0);
    const subsite_monitor::MonitorConfig config =
        subsite_monitor::load_config(config_path);
    assert(config.access_log == "/var/log/nginx/access.log");

    assert(chmod(config_path.c_str(), S_IRUSR | S_IWUSR | S_IRGRP) == 0);
    assert(throws_with("root only", config_path));
    assert(chmod(config_path.c_str(), S_IRUSR | S_IWUSR) == 0);
    {
        std::ofstream output{config_path, std::ios::trunc};
        output << "unexpected_key = value\n";
        assert(output.good());
    }
    assert(throws_with("unknown configuration key", config_path));

    std::filesystem::remove_all(directory);
    return 0;
}
