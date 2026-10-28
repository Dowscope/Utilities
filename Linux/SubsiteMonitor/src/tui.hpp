#pragma once

#include <filesystem>

namespace subsite_monitor {

// Runs the interactive statistics screen for the selected log file.
int run_terminal_ui(const std::filesystem::path& log_path,
                   const std::filesystem::path& nginx_config,
                   const std::filesystem::path& statistics_directory);

} // namespace subsite_monitor
