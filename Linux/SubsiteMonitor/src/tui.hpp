#pragma once

#include <filesystem>

namespace subsite_monitor {

// Runs the interactive statistics screen for the selected log file.
int run_terminal_ui(const std::filesystem::path& log_path);

} // namespace subsite_monitor
