#include "updater.hpp"

#include <cassert>
#include <string_view>

int main() {
    using subsite_monitor::parse_release_catalog;
    using subsite_monitor::version_is_newer;

    constexpr std::string_view catalog =
        "0.2.0-alpha.10\t2026-10-30\tU3Vic2l0ZSBNb25pdG9yIDI=\t"
        "Rmlyc3QgbGluZQpTZWNvbmQgbGluZQ==\n"
        "not-a-version\t2026-10-30\tVGl0bGU=\tTm90ZXM=\n"
        "0.3.0\t2026-10-31\tVGl0bGU=\t%%%\n";
    const auto releases = parse_release_catalog(catalog);
    assert(releases.size() == 1U);
    assert(releases.front().version == "0.2.0-alpha.10");
    assert(releases.front().published_date == "2026-10-30");
    assert(releases.front().title == "Subsite Monitor 2");
    assert(releases.front().notes == "First line\nSecond line");

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
