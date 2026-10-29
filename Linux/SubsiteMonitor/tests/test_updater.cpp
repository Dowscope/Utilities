#include "updater.hpp"

#include <cassert>
#include <string_view>

int main() {
    using subsite_monitor::parse_release_catalog;
    using subsite_monitor::version_is_newer;

    constexpr std::string_view catalog =
        "0.2.0-alpha.9\t2026-10-29\tVGl0bGUgOQ==\tTm90ZXMgOQ==\n"
        "0.2.0-alpha.10\t2026-10-30\tU3Vic2l0ZSBNb25pdG9yIDI=\t"
        "Rmlyc3QgbGluZQpTZWNvbmQgbGluZQ==\n"
        "0.2.0\t2026-10-31\tU3Vic2l0ZSBNb25pdG9yIDAuMg==\tU3RhYmxl\n"
        "not-a-version\t2026-10-30\tVGl0bGU=\tTm90ZXM=\n"
        "0.3.0\t2026-10-31\tVGl0bGU=\t%%%\n";
    const auto releases = parse_release_catalog(catalog);
    assert(releases.size() == 3U);
    assert(releases[0].version == "0.2.0");
    assert(releases[1].version == "0.2.0-alpha.10");
    assert(releases[1].published_date == "2026-10-30");
    assert(releases[1].title == "Subsite Monitor 2");
    assert(releases[1].notes == "First line\nSecond line");
    assert(releases[2].version == "0.2.0-alpha.9");

    assert(version_is_newer("0.1.0-alpha.10", "0.1.0-alpha.9"));
    assert(version_is_newer("0.1.1-alpha.1", "0.1.0-alpha.9"));
    assert(version_is_newer("0.1.0", "0.1.0-alpha.9"));
    assert(!version_is_newer("0.1.0-alpha.9", "0.1.0-alpha.10"));
    assert(!version_is_newer("0.1.0-alpha.6", "0.1.0-alpha.7"));
    assert(!version_is_newer("not-a-version", "0.1.0-alpha.7"));

    bool rejected_large_catalog = false;
    try {
        (void)parse_release_catalog(std::string(300000U, 'x'));
    } catch (...) {
        rejected_large_catalog = true;
    }
    assert(rejected_large_catalog);
}
