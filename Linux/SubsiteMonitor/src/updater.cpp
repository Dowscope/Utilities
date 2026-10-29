#include "updater.hpp"

#include <algorithm>
#include <array>
#include <cerrno>
#include <charconv>
#include <csignal>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fcntl.h>
#include <limits>
#include <stdexcept>
#include <string>
#include <string_view>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <vector>

namespace subsite_monitor {
namespace {

constexpr std::size_t maximum_catalog_size = 256U * 1024U;
constexpr std::string_view release_catalog_script = R"PYTHON(
import base64
import json
import re
import sys
import urllib.error
import urllib.request

url = "https://api.github.com/repos/Dowscope/Utilities/releases?per_page=100"
request = urllib.request.Request(
    url,
    headers={"Accept": "application/vnd.github+json",
             "User-Agent": "subsite-monitor-ui"},
)
try:
    with urllib.request.urlopen(request, timeout=15) as response:
        raw = response.read(4 * 1024 * 1024 + 1)
    if len(raw) > 4 * 1024 * 1024:
        raise ValueError("release catalog exceeds 4 MiB")
    releases = json.loads(raw)
except (OSError, ValueError, urllib.error.URLError, json.JSONDecodeError) as error:
    print("release catalog request failed: " + str(error), file=sys.stderr)
    sys.exit(1)

if not isinstance(releases, list):
    sys.exit(1)

version_pattern = re.compile(r"[0-9]+\.[0-9]+\.[0-9]+(?:-[0-9A-Za-z.-]+)?\Z")
digest_pattern = re.compile(r"sha256:[0-9a-fA-F]{64}\Z")
available = []

def version_key(version):
    core, separator, prerelease = version.partition("-")
    core_key = tuple(int(part) for part in core.split("."))
    if not separator:
        return core_key, 1, ()
    prerelease_key = tuple(
        (0, int(part)) if part.isdigit() else (1, part)
        for part in prerelease.split(".")
    )
    return core_key, 0, prerelease_key

for release in releases:
    if not isinstance(release, dict) or release.get("draft") is True:
        continue
    tag = release.get("tag_name", "")
    version = tag[1:] if isinstance(tag, str) and tag.startswith("v") else tag
    if not isinstance(version, str) or not version_pattern.fullmatch(version):
        continue
    assets = release.get("assets", [])
    if not isinstance(assets, list):
        continue
    archive_name = "subsite-monitor-" + version + "-linux-x86_64-gnu.tar.gz"
    archive = next((item for item in assets
                    if isinstance(item, dict) and item.get("name") == archive_name), None)
    has_checksums = any(isinstance(item, dict) and item.get("name") == "SHA256SUMS"
                        for item in assets)
    digest = archive.get("digest") if isinstance(archive, dict) else None
    if not has_checksums or not isinstance(digest, str) or not digest_pattern.fullmatch(digest):
        continue
    published = release.get("published_at", "")
    title = release.get("name", "")
    notes = release.get("body", "")
    if not isinstance(published, str) or not isinstance(title, str) or not isinstance(notes, str):
        continue
    date = published[:10]
    title = title[:256]
    notes = notes[:12000]
    fields = (version, date,
              base64.b64encode(title.encode("utf-8")).decode("ascii"),
              base64.b64encode(notes.encode("utf-8")).decode("ascii"))
    available.append(fields)

available.sort(key=lambda release: version_key(release[0]), reverse=True)
for fields in available[:10]:
    print("\t".join(fields))
)PYTHON";

struct NumericVersion {
    std::array<std::uint64_t, 3> core{};
    std::string_view prerelease;
};

bool parse_numeric_component(std::string_view text, std::uint64_t& value) {
    if (text.empty() || (text.size() > 1U && text.front() == '0')) {
        return false;
    }
    const auto [end, error] =
        std::from_chars(text.data(), text.data() + text.size(), value);
    return error == std::errc{} && end == text.data() + text.size();
}

bool parse_version(std::string_view text, NumericVersion& version) {
    const std::size_t dash = text.find('-');
    const std::string_view core = text.substr(0, dash);
    version.prerelease = dash == std::string_view::npos
                             ? std::string_view{}
                             : text.substr(dash + 1U);
    if (dash != std::string_view::npos && version.prerelease.empty()) {
        return false;
    }
    std::size_t start = 0;
    for (std::size_t component = 0; component < version.core.size(); ++component) {
        const std::size_t dot = core.find('.', start);
        const std::size_t end = dot == std::string_view::npos ? core.size() : dot;
        if (!parse_numeric_component(core.substr(start, end - start),
                                      version.core[component])) {
            return false;
        }
        if (component + 1U < version.core.size() && dot == std::string_view::npos) {
            return false;
        }
        if (component + 1U == version.core.size() && dot != std::string_view::npos) {
            return false;
        }
        start = end + 1U;
    }
    for (const char character : version.prerelease) {
        const bool valid = (character >= '0' && character <= '9') ||
                           (character >= 'A' && character <= 'Z') ||
                           (character >= 'a' && character <= 'z') ||
                           character == '.' || character == '-';
        if (!valid) return false;
    }
    return true;
}

int compare_prerelease(std::string_view left, std::string_view right) {
    if (left.empty() || right.empty()) {
        if (left.empty() == right.empty()) return 0;
        return left.empty() ? 1 : -1;
    }
    while (true) {
        const std::size_t left_dot = left.find('.');
        const std::size_t right_dot = right.find('.');
        const std::string_view left_part = left.substr(0, left_dot);
        const std::string_view right_part = right.substr(0, right_dot);
        const bool left_numeric = !left_part.empty() &&
            left_part.find_first_not_of("0123456789") == std::string_view::npos;
        const bool right_numeric = !right_part.empty() &&
            right_part.find_first_not_of("0123456789") == std::string_view::npos;
        if (left_numeric && right_numeric) {
            std::uint64_t left_number = 0;
            std::uint64_t right_number = 0;
            const bool left_ok = parse_numeric_component(left_part, left_number);
            const bool right_ok = parse_numeric_component(right_part, right_number);
            if (left_ok && right_ok && left_number != right_number) {
                return left_number < right_number ? -1 : 1;
            }
            if (left_ok != right_ok) return left_ok ? 1 : -1;
        } else if (left_numeric != right_numeric) {
            return left_numeric ? -1 : 1;
        } else if (left_part != right_part) {
            return left_part < right_part ? -1 : 1;
        }

        if (left_dot == std::string_view::npos ||
            right_dot == std::string_view::npos) {
            if (left_dot == right_dot) return 0;
            return left_dot == std::string_view::npos ? -1 : 1;
        }
        left.remove_prefix(left_dot + 1U);
        right.remove_prefix(right_dot + 1U);
    }
}

int wait_for_process(pid_t process) {
    int status = 0;
    while (waitpid(process, &status, 0) == -1) {
        if (errno != EINTR) return 127;
    }
    if (WIFEXITED(status)) return WEXITSTATUS(status);
    if (WIFSIGNALED(status)) return 128 + WTERMSIG(status);
    return 127;
}

std::string run_catalog_helper() {
    int output_pipe[2]{};
    if (pipe(output_pipe) != 0) {
        throw std::runtime_error("cannot create release-catalog pipe");
    }
    const pid_t child = fork();
    if (child == -1) {
        close(output_pipe[0]);
        close(output_pipe[1]);
        throw std::runtime_error("cannot start release-catalog helper");
    }
    if (child == 0) {
        close(output_pipe[0]);
        if (dup2(output_pipe[1], STDOUT_FILENO) == -1) _exit(127);
        close(output_pipe[1]);
        const int null_fd = open("/dev/null", O_WRONLY);
        if (null_fd >= 0) {
            (void)dup2(null_fd, STDERR_FILENO);
            close(null_fd);
        }
        execlp("python3", "python3", "-c", release_catalog_script.data(),
               static_cast<char*>(nullptr));
        _exit(127);
    }

    close(output_pipe[1]);
    std::string output;
    std::array<char, 4096> buffer{};
    bool too_large = false;
    while (true) {
        const ssize_t count = read(output_pipe[0], buffer.data(), buffer.size());
        if (count == 0) break;
        if (count < 0) {
            if (errno == EINTR) continue;
            close(output_pipe[0]);
            (void)wait_for_process(child);
            throw std::runtime_error("cannot read release-catalog response");
        }
        const auto amount = static_cast<std::size_t>(count);
        if (amount > maximum_catalog_size - output.size()) {
            too_large = true;
        } else if (!too_large) {
            output.append(buffer.data(), amount);
        }
    }
    close(output_pipe[0]);
    const int status = wait_for_process(child);
    if (too_large) {
        throw std::runtime_error("release-catalog response is too large");
    }
    if (status != 0) {
        if (status == 127) {
            throw std::runtime_error("python3 is required to check releases");
        }
        throw std::runtime_error(
            "cannot reach GitHub releases; check network access and try again");
    }
    return output;
}

int decode_base64_value(char character) {
    if (character >= 'A' && character <= 'Z') return character - 'A';
    if (character >= 'a' && character <= 'z') return character - 'a' + 26;
    if (character >= '0' && character <= '9') return character - '0' + 52;
    if (character == '+') return 62;
    if (character == '/') return 63;
    return -1;
}

std::string decode_base64(std::string_view encoded) {
    std::string decoded;
    decoded.reserve(encoded.size() * 3U / 4U);
    std::uint32_t accumulator = 0;
    int bits = 0;
    for (const char character : encoded) {
        if (character == '=') break;
        const int value = decode_base64_value(character);
        if (value < 0) {
            throw std::runtime_error("malformed release-catalog data");
        }
        accumulator = (accumulator << 6U) | static_cast<std::uint32_t>(value);
        bits += 6;
        if (bits >= 8) {
            bits -= 8;
            decoded.push_back(static_cast<char>((accumulator >> bits) & 0xffU));
        }
    }
    return decoded;
}

bool valid_version(std::string_view text) {
    NumericVersion parsed;
    return parse_version(text, parsed);
}

bool valid_release_date(std::string_view date) {
    if (date.size() != 10U || date[4] != '-' || date[7] != '-') return false;
    for (std::size_t index = 0; index < date.size(); ++index) {
        if (index == 4U || index == 7U) continue;
        if (date[index] < '0' || date[index] > '9') return false;
    }
    return true;
}

} // namespace

std::vector<SoftwareRelease> parse_release_catalog(std::string_view catalog) {
    if (catalog.size() > maximum_catalog_size) {
        throw std::runtime_error("release-catalog response is too large");
    }
    std::vector<SoftwareRelease> releases;
    std::size_t line_start = 0;
    while (line_start < catalog.size()) {
        const std::size_t line_end = catalog.find('\n', line_start);
        const std::size_t end = line_end == std::string_view::npos
                                    ? catalog.size()
                                    : line_end;
        std::string_view line = catalog.substr(line_start, end - line_start);
        if (!line.empty() && line.back() == '\r') line.remove_suffix(1U);
        line_start = line_end == std::string_view::npos ? catalog.size()
                                                        : line_end + 1U;
        if (line.empty()) continue;
        std::array<std::string_view, 4> fields{};
        std::size_t field_start = 0;
        bool malformed = false;
        for (std::size_t index = 0; index < fields.size(); ++index) {
            const std::size_t separator = line.find('\t', field_start);
            if (index + 1U == fields.size()) {
                if (separator != std::string_view::npos) malformed = true;
                fields[index] = line.substr(field_start);
            } else if (separator == std::string_view::npos) {
                malformed = true;
                break;
            } else {
                fields[index] = line.substr(field_start, separator - field_start);
                field_start = separator + 1U;
            }
        }
        if (malformed || !valid_version(fields[0]) ||
            !valid_release_date(fields[1])) {
            continue;
        }
        try {
            releases.push_back({std::string{fields[0]}, std::string{fields[1]},
                                decode_base64(fields[2]),
                                decode_base64(fields[3])});
        } catch (const std::runtime_error&) {
            continue;
        }
        if (releases.size() == 10U) break;
    }
    std::stable_sort(releases.begin(), releases.end(),
                     [](const SoftwareRelease& left,
                        const SoftwareRelease& right) {
                         return version_is_newer(left.version, right.version);
                     });
    if (releases.size() > 10U) releases.resize(10U);
    return releases;
}

bool version_is_newer(std::string_view candidate, std::string_view current) {
    NumericVersion left;
    NumericVersion right;
    if (!parse_version(candidate, left) || !parse_version(current, right)) {
        return false;
    }
    if (left.core != right.core) return left.core > right.core;
    return compare_prerelease(left.prerelease, right.prerelease) > 0;
}

std::vector<SoftwareRelease> fetch_software_releases() {
    return parse_release_catalog(run_catalog_helper());
}

int install_software_release(std::string_view version,
                             const std::filesystem::path& installer_path) {
    if (!valid_version(version) || !installer_path.is_absolute()) return 2;
    const std::string version_argument{version};
    const std::string installer_argument = installer_path.string();
    const pid_t child = fork();
    if (child == -1) return 127;
    if (child == 0) {
        if (geteuid() == 0) {
            execl(installer_argument.c_str(), installer_argument.c_str(),
                  "--version", version_argument.c_str(),
                  static_cast<char*>(nullptr));
            (void)dprintf(STDERR_FILENO,
                          "subsite-monitor: cannot run updater installer: %s\n",
                          std::strerror(errno));
        } else {
            execlp("sudo", "sudo", installer_argument.c_str(), "--version",
                   version_argument.c_str(), static_cast<char*>(nullptr));
            (void)dprintf(STDERR_FILENO,
                          "subsite-monitor: sudo is required to install updates: %s\n",
                          std::strerror(errno));
        }
        _exit(127);
    }
    return wait_for_process(child);
}

} // namespace subsite_monitor
