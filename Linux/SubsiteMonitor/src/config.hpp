#pragma once

#include <filesystem>
#include <string_view>

namespace subsite_monitor {

struct MonitorConfig {
    std::filesystem::path access_log;
};

[[nodiscard]] MonitorConfig parse_config_text(std::string_view text);

// Reads the root-owned, owner-readable configuration file. The file and its
// parent directory must not be writable by group or other users.
[[nodiscard]] MonitorConfig load_config(const std::filesystem::path& path);

} // namespace subsite_monitor
