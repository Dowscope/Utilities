#pragma once

#include <filesystem>

namespace subsite_monitor {

// Runs the foreground log monitor intended for systemd supervision.
int run_monitor_service(const std::filesystem::path& log_path,
                       const std::filesystem::path& statistics_directory);

} // namespace subsite_monitor
