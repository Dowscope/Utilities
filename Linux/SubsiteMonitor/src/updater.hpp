#pragma once

#include <filesystem>
#include <string>
#include <string_view>
#include <vector>

namespace subsite_monitor {

struct SoftwareRelease {
    std::string version;
    std::string published_date;
    std::string title;
    std::string notes;
};

// Parses the bounded tab-separated output from the standard-library Python
// release-list helper. Notes and titles are base64 encoded in that format.
std::vector<SoftwareRelease> parse_release_catalog(std::string_view catalog);

// Compares supported numeric semantic versions, including alpha prereleases.
bool version_is_newer(std::string_view candidate, std::string_view current);

// Fetches up to ten GitHub releases that have the supported Linux archive and
// an API-provided SHA-256 digest. The installer rechecks that digest.
std::vector<SoftwareRelease> fetch_software_releases();

// Executes the installed release installer for a previously confirmed version.
// Returns the installer process exit status (127 if it could not be launched).
int install_software_release(std::string_view version,
                             const std::filesystem::path& installer_path);

} // namespace subsite_monitor
