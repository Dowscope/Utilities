#include "statistics.hpp"
#include "storage.hpp"
#include "report.hpp"

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <string_view>
#include <vector>

#include <sys/stat.h>
#include <unistd.h>

namespace {

void require(bool condition, std::string_view message) {
    if (!condition) {
        std::cerr << "Storage test failed: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

class TemporaryDirectory {
public:
    TemporaryDirectory() {
        std::string pattern =
            (std::filesystem::temp_directory_path() /
             "subsite-monitor-storage-XXXXXX").string();
        std::vector<char> writable(pattern.begin(), pattern.end());
        writable.push_back('\0');
        const char* created = mkdtemp(writable.data());
        if (created == nullptr) {
            std::cerr << "Storage test failed: cannot create temporary directory\n";
            std::exit(EXIT_FAILURE);
        }
        path_ = created;
    }

    TemporaryDirectory(const TemporaryDirectory&) = delete;
    TemporaryDirectory& operator=(const TemporaryDirectory&) = delete;

    ~TemporaryDirectory() {
        std::error_code error;
        std::filesystem::remove_all(path_, error);
    }

    [[nodiscard]] const std::filesystem::path& path() const { return path_; }

private:
    std::filesystem::path path_;
};

unsigned mode_of(const std::filesystem::path& path) {
    struct stat status {};
    require(stat(path.c_str(), &status) == 0, "inspect storage permissions");
    return static_cast<unsigned>(status.st_mode & 0777);
}

} // namespace

int main() {
    TemporaryDirectory temporary;
    const std::filesystem::path store = temporary.path() / "stats";

    (void)subsite_monitor::collect_and_store("tests/fixtures/access.log", store);
    (void)subsite_monitor::collect_and_store("tests/fixtures/access.log", store);

    require(mode_of(store) == 0700U, "restrict the statistics directory");
    require(mode_of(store / "2026-10-26.csv") == 0600U,
            "restrict daily CSV permissions");
    require(mode_of(store / "2026-10-27.csv") == 0600U,
            "restrict the second daily CSV permissions");
    require(mode_of(store / "requests-2026-10-26.csv") == 0600U &&
                mode_of(store / "requests-2026-10-27.csv") == 0600U,
            "restrict per-request CSV permissions");

    const subsite_monitor::StatisticsReport loaded =
        subsite_monitor::read_stored_statistics(store);
    require(loaded.sites.size() == 2, "load both websites from daily CSV files");
    require(loaded.malformed_lines == 0,
            "CSV storage contains valid statistics only");
    const auto live = loaded.sites.find("live.lakeheadschools.ca");
    require(live != loaded.sites.end() && live->second.requests == 4,
            "repeated collection replaces snapshots without double counting");
    require(live->second.requests_by_day.at("2026-10-26") == 1 &&
                live->second.requests_by_day.at("2026-10-27") == 3,
            "read per-day request counts");

    const subsite_monitor::StatisticsReport latest_day =
        subsite_monitor::read_stored_statistics(store, 1);
    require(latest_day.sites.size() == 2 &&
                latest_day.sites.at("live.lakeheadschools.ca").requests == 3 &&
                latest_day.sites.at("test.lakeheadschools.ca").requests == 1,
            "limit reports to the most recent stored date");

    const std::vector<subsite_monitor::SiteDayHistory> history =
        subsite_monitor::read_site_history(store,
                                           "live.lakeheadschools.ca");
    require(history.size() == 2 && history[0].date == "2026-10-27" &&
                history[0].requests == 3 && history[1].date == "2026-10-26" &&
                history[1].requests == 1,
            "read selected site's CSV rows newest first");

    const subsite_monitor::RequestPage first_requests =
        subsite_monitor::read_site_requests(
            store, "live.lakeheadschools.ca", 0, 2);
    require(first_requests.records.size() == 2 && first_requests.has_more,
            "read a bounded page of actual request records");
    require(subsite_monitor::format_timestamp_utc(
                first_requests.records[0].timestamp_utc) ==
                "2026-10-26T13:03:01Z" &&
                first_requests.records[0].client_ip == "2001:db8::2" &&
                first_requests.records[0].method == "POST" &&
                first_requests.records[0].url == "/forms/submit" &&
                first_requests.records[0].status == 500,
            "retain request timestamp, client IP, method, URL, and status");

    const subsite_monitor::RequestPage query_request =
        subsite_monitor::read_site_requests(
            store, "test.lakeheadschools.ca", 0, 10);
    require(query_request.records.size() == 1 &&
                query_request.records[0].url == "/private?token=hidden" &&
                query_request.records[0].client_ip == "203.0.113.11" &&
                !query_request.has_more,
            "preserve complete URLs including query strings and client IPs");

    const std::filesystem::path paging_log = temporary.path() / "paging.log";
    {
        std::ofstream output{paging_log};
        require(static_cast<bool>(output), "create request paging fixture");
        for (int index = 0; index < 105; ++index) {
            output << "2026-10-28T10:00:00+00:00 paging.example.test "
                   << "192.0.2.10 \"GET /item?index=" << index;
            if (index == 0) {
                output << "&tag=a,b";
            }
            output << " HTTP/1.1\" 200\n";
        }
    }
    (void)subsite_monitor::collect_and_store(paging_log, store);
    const subsite_monitor::RequestPage first_page =
        subsite_monitor::read_site_requests(store, "paging.example.test", 0,
                                            100);
    const subsite_monitor::RequestPage second_page =
        subsite_monitor::read_site_requests(store, "paging.example.test", 100,
                                            100);
    require(first_page.records.size() == 100 && first_page.has_more &&
                first_page.records.front().url ==
                    "/item?index=0&tag=a,b",
            "paginate a complete event list and correctly escape CSV commas");
    require(second_page.records.size() == 5 && !second_page.has_more &&
                second_page.records.back().url == "/item?index=104",
            "read every request across the final page");

    std::cout << "Storage tests passed\n";
}
