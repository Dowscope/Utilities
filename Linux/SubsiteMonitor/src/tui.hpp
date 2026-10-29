#pragma once

#include <filesystem>
#include <span>

namespace subsite_monitor {

// Runs the interactive statistics screen for the selected log file.
int run_terminal_ui(const std::filesystem::path& log_path,
                   const std::filesystem::path& nginx_config,
                   const std::filesystem::path& statistics_directory,
                   std::span<char* const> startup_arguments);

} // namespace subsite_monitor
