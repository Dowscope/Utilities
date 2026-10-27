#pragma once

#include <filesystem>
#include <string>
#include <vector>

namespace subsite_monitor {

// Finds literal and wildcard server_name values in an Nginx config and its
// supported include files. Regex server names are intentionally omitted.
[[nodiscard]] std::vector<std::string> discover_nginx_server_names(
    const std::filesystem::path& main_config);

} // namespace subsite_monitor
