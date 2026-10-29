#include "tui.hpp"

#include "report.hpp"
#include "nginx_config.hpp"
#include "statistics.hpp"
#include "storage.hpp"
#include "updater.hpp"

#include <algorithm>
#include <exception>
#include <filesystem>
#include <limits>
#include <memory>
#include <mutex>
#include <sstream>
#include <string>
#include <string_view>
#include <stdexcept>
#include <thread>
#include <utility>
#include <vector>

#include <ncurses.h>
#include <unistd.h>

namespace subsite_monitor {
namespace {

#ifndef SUBSITE_MONITOR_VERSION
#define SUBSITE_MONITOR_VERSION "0.1.0-alpha.5"
#endif

#ifndef SUBSITE_MONITOR_INSTALLER_PATH
#define SUBSITE_MONITOR_INSTALLER_PATH "/usr/share/subsite-monitor/installer.sh"
#endif

constexpr std::string_view current_version = SUBSITE_MONITOR_VERSION;
const std::filesystem::path release_installer_path =
    SUBSITE_MONITOR_INSTALLER_PATH;

class CursesSession {
public:
    CursesSession() {
        if (initscr() == nullptr) {
            throw std::runtime_error("cannot initialize the terminal screen");
        }
        active_ = true;
        cbreak();
        noecho();
        keypad(stdscr, true);
        curs_set(0);
    }

    CursesSession(const CursesSession&) = delete;
    CursesSession& operator=(const CursesSession&) = delete;

    ~CursesSession() {
        if (active_) {
            endwin();
        }
    }

private:
    bool active_{};
};

struct SiteRow {
    std::string_view hostname;
    const SiteStatistics* statistics{};
};

struct UiSnapshot {
    StatisticsReport report;
    std::string status_message;
};

void write_at(int row, int column, std::string_view text, int max_width);

enum class UpdateCheckState { checking, available, current, failed };

struct UpdateCheckResult {
    std::mutex mutex;
    UpdateCheckState state{UpdateCheckState::checking};
    std::string version;
};

std::shared_ptr<UpdateCheckResult> start_update_check() {
    auto result = std::make_shared<UpdateCheckResult>();
    std::thread([result] {
        UpdateCheckState state = UpdateCheckState::current;
        std::string version;
        try {
            const std::vector<SoftwareRelease> releases =
                fetch_software_releases();
            for (const SoftwareRelease& release : releases) {
                if (version_is_newer(release.version, current_version)) {
                    state = UpdateCheckState::available;
                    version = release.version;
                    break;
                }
            }
        } catch (...) {
            state = UpdateCheckState::failed;
        }
        std::lock_guard lock{result->mutex};
        result->version = std::move(version);
        result->state = state;
    }).detach();
    return result;
}

std::string update_badge_text(
    const std::shared_ptr<UpdateCheckResult>& update_check) {
    std::lock_guard lock{update_check->mutex};
    if (update_check->state == UpdateCheckState::available) {
        return "[ Update " + update_check->version + " (u) ]";
    }
    return {};
}

void draw_update_badge(int rows, int columns,
                       const std::shared_ptr<UpdateCheckResult>& update_check) {
    constexpr int maximum_badge_width = 36;
    if (rows < 18 || columns < 48) return;
    const std::string label = update_badge_text(update_check);
    if (label.empty()) return;
    const int badge_width = std::min(
        maximum_badge_width, static_cast<int>(label.size()));
    const int left = columns - badge_width - 2;
    attron(COLOR_PAIR(4) | A_BOLD);
    write_at(rows - 3, left, label, badge_width);
    attroff(COLOR_PAIR(4) | A_BOLD);
}

enum class SiteSortColumn { hostname, requests, last_access };

int compare_site_field(const SiteRow& left, const SiteRow& right,
                       SiteSortColumn column) {
    switch (column) {
    case SiteSortColumn::hostname:
        return left.hostname.compare(right.hostname);
    case SiteSortColumn::requests:
        if (left.statistics->requests < right.statistics->requests) return -1;
        if (left.statistics->requests > right.statistics->requests) return 1;
        return 0;
    case SiteSortColumn::last_access:
        if (left.statistics->latest_access_utc <
            right.statistics->latest_access_utc) {
            return -1;
        }
        if (left.statistics->latest_access_utc >
            right.statistics->latest_access_utc) {
            return 1;
        }
        return 0;
    }
    return 0;
}

std::vector<SiteRow> sorted_sites(const StatisticsReport& report,
                                 SiteSortColumn sort_column,
                                 bool sort_ascending) {
    std::vector<SiteRow> sites;
    sites.reserve(report.sites.size());
    for (const auto& [hostname, statistics] : report.sites) {
        sites.push_back({hostname, &statistics});
    }
    std::sort(sites.begin(), sites.end(), [sort_column, sort_ascending](
                                               const SiteRow& left,
                                               const SiteRow& right) {
        const int order = compare_site_field(left, right, sort_column);
        if (order != 0) {
            return sort_ascending ? order < 0 : order > 0;
        }
        return left.hostname < right.hostname;
    });
    return sites;
}

UiSnapshot build_ui_statistics(
    const std::filesystem::path& log_path,
    const std::filesystem::path& nginx_config,
    const std::filesystem::path& statistics_directory) {
    UiSnapshot snapshot;
    try {
        if (monitor_service_is_active(statistics_directory)) {
            snapshot.report = read_stored_statistics(statistics_directory);
            snapshot.status_message =
                "Monitoring service active; CSV data updates automatically.";
        } else {
            const StatisticsReport current_report =
                collect_and_store(log_path, statistics_directory);
            snapshot.report = read_stored_statistics(statistics_directory);
            snapshot.report.malformed_lines = current_report.malformed_lines;
        }
    } catch (const std::runtime_error& error) {
        const std::string_view message{error.what()};
        if (message.starts_with("monitor service is running;")) {
            snapshot.status_message =
                "Monitoring service active; CSV data updates automatically.";
            snapshot.report = read_stored_statistics(statistics_directory);
        } else {
            if (nginx_config.empty() ||
                !message.starts_with("cannot open access log:")) {
                throw;
            }
            snapshot.status_message =
                "Access log unavailable; showing configured sites only.";
            snapshot.report = read_stored_statistics(statistics_directory);
        }
    }
    if (nginx_config.empty()) {
        return snapshot;
    }

    constexpr std::size_t max_ui_sites = 10000U;
    for (const std::string& hostname :
         discover_nginx_server_names(nginx_config)) {
        if (snapshot.report.sites.contains(hostname)) {
            continue;
        }
        if (snapshot.report.sites.size() >= max_ui_sites) {
            throw std::runtime_error(
                "statistics limit exceeded: too many configured and observed websites");
        }
        snapshot.report.sites.emplace(hostname, SiteStatistics{});
    }
    return snapshot;
}

void write_at(int row, int column, std::string_view text, int max_width) {
    if (row < 0 || column < 0 || max_width <= 0 || text.empty()) {
        return;
    }
    const int text_width = static_cast<int>(std::min(
        text.size(), static_cast<std::size_t>(max_width)));
    mvaddnstr(row, column, text.data(), text_width);
}

std::string safe_path_label(const std::filesystem::path& path) {
    std::string label = path.filename().string();
    for (char& character : label) {
        const auto byte = static_cast<unsigned char>(character);
        if (byte < 0x20U || byte == 0x7fU) {
            character = '?';
        }
    }
    constexpr std::size_t max_label_size = 80;
    if (label.size() > max_label_size) {
        label.resize(max_label_size - 3);
        label += "...";
    }
    return label;
}

std::string safe_terminal_text(std::string_view text) {
    std::string safe{text};
    for (char& character : safe) {
        const auto byte = static_cast<unsigned char>(character);
        if (byte < 0x20U || byte == 0x7fU) {
            character = '?';
        }
    }
    return safe;
}

std::size_t count_site_requests(
    const std::filesystem::path& statistics_directory,
    const std::string& hostname) {
    std::size_t total = 0;
    for (const SiteDayHistory& day :
         read_site_history(statistics_directory, hostname)) {
        const std::size_t remaining =
            std::numeric_limits<std::size_t>::max() - total;
        if (day.requests > static_cast<std::uint64_t>(remaining)) {
            throw std::runtime_error("site request history exceeds supported limits");
        }
        total += static_cast<std::size_t>(day.requests);
    }
    return total;
}

RequestPage read_newest_requests(
    const std::filesystem::path& statistics_directory,
    const std::string& hostname, std::size_t page_offset,
    std::size_t page_size) {
    RequestPage page = read_site_requests(statistics_directory, hostname,
                                          page_offset, page_size);
    std::reverse(page.records.begin(), page.records.end());
    page.offset = page_offset;
    return page;
}

enum class RequestSortColumn { timestamp, client_ip, method_status, url };

int compare_request_field(const RequestRecord& left,
                          const RequestRecord& right,
                          RequestSortColumn column) {
    switch (column) {
    case RequestSortColumn::timestamp:
        if (left.timestamp_utc < right.timestamp_utc) return -1;
        if (left.timestamp_utc > right.timestamp_utc) return 1;
        return 0;
    case RequestSortColumn::client_ip:
        return left.client_ip.compare(right.client_ip);
    case RequestSortColumn::method_status:
        if (const int method_order = left.method.compare(right.method);
            method_order != 0) {
            return method_order;
        }
        if (left.status < right.status) return -1;
        if (left.status > right.status) return 1;
        return 0;
    case RequestSortColumn::url:
        return left.url.compare(right.url);
    }
    return 0;
}

void sort_request_page(RequestPage& page, RequestSortColumn column,
                       bool ascending) {
    std::stable_sort(page.records.begin(), page.records.end(),
                     [column, ascending](const RequestRecord& left,
                                         const RequestRecord& right) {
        const int order = compare_request_field(left, right, column);
        return ascending ? order < 0 : order > 0;
    });
}

std::string sort_direction(RequestSortColumn column,
                           RequestSortColumn active_column,
                           bool ascending) {
    if (column != active_column) {
        return {};
    }
    return ascending ? " ^" : " v";
}

std::string sort_direction(SiteSortColumn column,
                           SiteSortColumn active_column, bool ascending) {
    if (column != active_column) {
        return {};
    }
    return ascending ? " ^" : " v";
}

void configure_colors() {
    if (has_colors() == false || start_color() == ERR) {
        return;
    }
    use_default_colors();
    init_pair(1, COLOR_WHITE, COLOR_BLUE);
    init_pair(2, COLOR_WHITE, COLOR_BLUE);
    init_pair(3, COLOR_CYAN, -1);
    init_pair(4, COLOR_GREEN, -1);
    init_pair(5, COLOR_YELLOW, -1);
}

void draw_screen(const StatisticsReport& report,
                 const std::filesystem::path& log_path, std::size_t selected,
                 SiteSortColumn sort_column, bool sort_ascending,
                 std::string_view status_message) {
    int rows = 0;
    int columns = 0;
    getmaxyx(stdscr, rows, columns);
    erase();

    if (rows < 18 || columns < 64) {
        write_at(0, 0, "Terminal too small. Resize to at least 64 x 18.",
                 columns);
        write_at(std::max(1, rows - 1), 0, "q: quit", columns);
        refresh();
        return;
    }

    const std::vector<SiteRow> sites =
        sorted_sites(report, sort_column, sort_ascending);
    const int content_width = columns - 2;

    attron(COLOR_PAIR(1) | A_BOLD);
    mvhline(0, 0, ' ', columns);
    write_at(0, 2, "SUBSITE MONITOR", content_width - 2);
    const std::string source = "ACCESS LOG  " + safe_path_label(log_path);
    const int source_column = std::max(2, columns - static_cast<int>(source.size()) - 2);
    write_at(0, source_column, source, columns - source_column - 1);
    attroff(COLOR_PAIR(1) | A_BOLD);

    attron(COLOR_PAIR(3));
    write_at(1, 2, "WEBSITE ACTIVITY OVERVIEW", content_width);
    attroff(COLOR_PAIR(3));
    mvhline(2, 1, ACS_HLINE, content_width);

    std::uint64_t total_requests = 0;
    for (const auto& [hostname, statistics] : report.sites) {
        (void)hostname;
        total_requests += statistics.requests;
    }
    std::ostringstream summary;
    summary << "Requests  " << total_requests
            << "     Websites  " << report.sites.size()
            << "     Malformed lines  " << report.malformed_lines;
    attron(A_BOLD);
    write_at(3, 2, summary.str(), content_width);
    attroff(A_BOLD);
    mvhline(4, 1, ACS_HLINE, content_width);

    const int site_width = std::max(16, std::min(30, columns - 52));
    const int requests_column = 2 + site_width + 2;
    const int last_access_column = requests_column + 12;
    attron(A_BOLD);
    write_at(5, 2, "WEBSITE" +
                         sort_direction(SiteSortColumn::hostname, sort_column,
                                        sort_ascending),
             site_width);
    write_at(5, requests_column, "REQUESTS" +
                                      sort_direction(SiteSortColumn::requests,
                                                     sort_column,
                                                     sort_ascending),
             10);
    write_at(5, last_access_column,
             "LAST ACCESS (SERVER TIME)" +
                 sort_direction(SiteSortColumn::last_access, sort_column,
                                sort_ascending),
             columns - last_access_column - 2);
    attroff(A_BOLD);
    mvhline(6, 1, ACS_HLINE, content_width);

    const int first_site_row = 7;
    const int daily_header_row = rows - 6;
    const int visible_sites =
        std::max(0, daily_header_row - 1 - first_site_row);
    std::size_t first_visible = 0;
    if (selected >= static_cast<std::size_t>(visible_sites) && visible_sites > 0) {
        first_visible = selected - static_cast<std::size_t>(visible_sites) + 1;
    }

    for (int visible_row = 0; visible_row < visible_sites; ++visible_row) {
        const std::size_t site_index =
            first_visible + static_cast<std::size_t>(visible_row);
        if (site_index >= sites.size()) {
            break;
        }
        const SiteRow& site = sites[site_index];
        const int row = first_site_row + visible_row;
        if (site_index == selected) {
            attron(COLOR_PAIR(2) | A_BOLD);
            mvhline(row, 1, ' ', content_width);
        }

        write_at(row, 2, site.hostname, site_width);
        std::ostringstream requests;
        requests << site.statistics->requests;
        write_at(row, requests_column, requests.str(), 10);
        const std::string last_access = site.statistics->requests == 0
                                            ? "Never"
                                            : format_timestamp_local(
                                                  site.statistics->latest_access_utc);
        write_at(row, last_access_column, last_access,
                 columns - last_access_column - 2);

        if (site_index == selected) {
            attroff(COLOR_PAIR(2) | A_BOLD);
        }
    }

    mvhline(daily_header_row - 1, 1, ACS_HLINE, content_width);
    attron(COLOR_PAIR(3) | A_BOLD);
    write_at(daily_header_row, 2, "DAILY REQUESTS", content_width);
    attroff(COLOR_PAIR(3) | A_BOLD);
    const int daily_row = daily_header_row + 1;
    if (sites.empty()) {
        write_at(daily_row, 2, "No valid requests found in this log.",
                 content_width);
    } else {
        const SiteRow& site = sites[std::min(selected, sites.size() - 1)];
        if (site.statistics->requests == 0) {
            write_at(daily_row, 2, "No requests recorded for this configured site.",
                     content_width);
        } else {
        std::ostringstream daily;
        daily << site.hostname << "   ";
        bool first_day = true;
        for (const auto& [date, requests] : site.statistics->requests_by_day) {
            if (!first_day) {
                daily << "   |   ";
            }
            daily << date << ": " << requests;
            first_day = false;
        }
        attron(A_BOLD);
        write_at(daily_row, 2, daily.str(), content_width);
        attroff(A_BOLD);
        }
    }

    mvhline(rows - 2, 0, ' ', columns);
    attron(COLOR_PAIR(1));
    const std::string footer = columns >= 68
        ? " Sort: 1-3 | Select: Up/Dn | Details: Enter | Refresh: r | Quit: q"
        : "Sort:1-3 | Select:Up/Dn | Details:Enter | Refresh:r | Quit:q";
    write_at(rows - 2, 1, footer, columns - 2);
    attroff(COLOR_PAIR(1));
    if (!status_message.empty()) {
        attron(COLOR_PAIR(5) | A_BOLD);
        const std::string safe_message = safe_terminal_text(status_message);
        write_at(rows - 3, 2, safe_message, content_width);
        attroff(COLOR_PAIR(5) | A_BOLD);
    }
    refresh();
}

void draw_request_screen(std::string_view hostname, const RequestPage& page,
                         std::size_t total_requests,
                         RequestSortColumn sort_column, bool sort_ascending,
                         std::size_t selected,
                         std::size_t url_horizontal_offset,
                         std::string_view status_message) {
    int rows = 0;
    int columns = 0;
    getmaxyx(stdscr, rows, columns);
    erase();
    if (rows < 12 || columns < 70) {
        write_at(0, 0, "Terminal too small. Resize to at least 70 x 12.",
                 columns);
        write_at(std::max(1, rows - 1), 0, "Esc: return", columns);
        refresh();
        return;
    }

    const int content_width = columns - 2;
    attron(COLOR_PAIR(1) | A_BOLD);
    mvhline(0, 0, ' ', columns);
    write_at(0, 2, "REQUEST HISTORY", content_width);
    attroff(COLOR_PAIR(1) | A_BOLD);
    attron(COLOR_PAIR(3) | A_BOLD);
    write_at(1, 2, hostname, content_width);
    attroff(COLOR_PAIR(3) | A_BOLD);

    std::ostringstream range;
    if (!page.records.empty()) {
        range << "Requests " << page.offset + 1U << "-"
              << page.offset + page.records.size() << " of " << total_requests;
        if (sort_column == RequestSortColumn::timestamp && !sort_ascending) {
            range << " (newest first)";
        }
        if (page.has_more) {
            range << " (+ newer)";
        }
    } else {
        range << "No request records stored for this site";
    }
    write_at(2, 2, range.str(), content_width);

    std::size_t longest_ip = 0;
    std::size_t longest_method = 0;
    for (const RequestRecord& record : page.records) {
        longest_ip = std::max(longest_ip, record.client_ip.size());
        longest_method = std::max(longest_method, record.method.size());
    }
    const int time_width = 25;
    constexpr int column_gaps = 6;
    constexpr int minimum_url_width = 8;
    const int maximum_method_width = std::max(
        12, content_width - time_width - 16 - column_gaps - minimum_url_width);
    const int method_width = std::min(
        maximum_method_width,
        std::max(12, static_cast<int>(std::min<std::size_t>(
                         longest_method + 5U, 40U))));
    const int ip_width = static_cast<int>(std::min<std::size_t>(
        longest_ip,
        static_cast<std::size_t>(std::max(
            16, content_width - time_width - method_width - column_gaps -
                    minimum_url_width))));
    const int url_width = content_width - time_width - ip_width -
                          method_width - column_gaps;
    const int ip_column = 2 + time_width + 2;
    const int method_column = ip_column + ip_width + 2;
    const int url_column = method_column + method_width + 2;

    attron(A_BOLD);
    write_at(3, 2, "TIME" + sort_direction(RequestSortColumn::timestamp,
                                             sort_column, sort_ascending),
             time_width);
    write_at(3, ip_column, "IP" + sort_direction(RequestSortColumn::client_ip,
                                                   sort_column, sort_ascending),
             ip_width);
    write_at(3, method_column,
             "METHOD/STATUS" +
                 sort_direction(RequestSortColumn::method_status, sort_column,
                                sort_ascending),
             method_width);
    write_at(3, url_column, "URL" + sort_direction(RequestSortColumn::url,
                                                     sort_column, sort_ascending),
             url_width);
    attroff(A_BOLD);
    mvhline(4, 1, ACS_HLINE, content_width);

    const std::size_t visible_requests =
        static_cast<std::size_t>(std::max(1, rows - 8));
    const std::size_t first_visible =
        selected >= visible_requests ? selected - visible_requests + 1U : 0U;
    if (page.records.empty()) {
        write_at(5, 2, "Run a collection after confirming the access log.",
                 content_width);
    } else {
        for (std::size_t line = 0; line < visible_requests; ++line) {
            const std::size_t index = first_visible + line;
            if (index >= page.records.size()) {
                break;
            }
            const RequestRecord& record = page.records[index];
            const int row = 5 + static_cast<int>(line);
            if (index == selected) {
                attron(COLOR_PAIR(2) | A_BOLD);
                mvhline(row, 1, ' ', content_width);
            }

            write_at(row, 2, format_timestamp_local(record.timestamp_utc),
                     time_width);
            write_at(row, ip_column, record.client_ip, ip_width);
            std::ostringstream method_status;
            method_status << record.method << " " << record.status;
            write_at(row, method_column, method_status.str(), method_width);
            std::string safe_url = safe_terminal_text(record.url);
            if (index == selected && url_horizontal_offset < safe_url.size()) {
                safe_url.erase(0, url_horizontal_offset);
            } else if (index != selected) {
                const std::size_t width =
                    static_cast<std::size_t>(std::max(0, url_width));
                if (safe_url.size() > width && width > 3U) {
                    safe_url.resize(width - 3U);
                    safe_url += "...";
                }
            }
            write_at(row, url_column, safe_url, url_width);

            if (index == selected) {
                attroff(COLOR_PAIR(2) | A_BOLD);
            }
        }
    }

    mvhline(rows - 2, 0, ' ', columns);
    attron(COLOR_PAIR(1));
    std::ostringstream footer;
    if (columns >= 100) {
        footer << " Sort: 1-4 " << (sort_ascending ? "ASC" : "DESC")
               << " | Rows: Up/Dn | Pages: PgUp/Dn | URL: L/R | Back: Esc | Quit: q";
    } else {
        footer << "Sort:1-4 | Rows:Up/Dn | Page:PgUp/Dn | URL:L/R | Back:Esc | Quit:q";
    }
    write_at(rows - 2, 1, footer.str(), columns - 2);
    attroff(COLOR_PAIR(1));
    if (!status_message.empty()) {
        attron(COLOR_PAIR(5) | A_BOLD);
        write_at(rows - 3, 2, safe_terminal_text(status_message), content_width);
        attroff(COLOR_PAIR(5) | A_BOLD);
    }
    refresh();
}

std::vector<std::string> release_note_lines(std::string_view notes,
                                            std::size_t width) {
    std::vector<std::string> lines;
    std::istringstream input{std::string{notes}};
    std::string line;
    while (std::getline(input, line)) {
        std::string safe_line = safe_terminal_text(line);
        if (safe_line.empty()) {
            lines.emplace_back();
            continue;
        }
        while (safe_line.size() > width) {
            std::size_t split = safe_line.rfind(' ', width);
            if (split == std::string::npos || split == 0U) split = width;
            lines.emplace_back(safe_line.substr(0, split));
            safe_line.erase(0, split);
            while (!safe_line.empty() && safe_line.front() == ' ') {
                safe_line.erase(0, 1U);
            }
        }
        lines.push_back(std::move(safe_line));
    }
    if (lines.empty()) lines.emplace_back("No release notes were provided.");
    return lines;
}

bool is_enter_key(int key);

void draw_update_screen(const std::vector<SoftwareRelease>& releases,
                        std::size_t selected, std::size_t notes_offset,
                        std::string_view message) {
    int rows = 0;
    int columns = 0;
    getmaxyx(stdscr, rows, columns);
    erase();
    if (rows < 18 || columns < 64) {
        write_at(0, 0, "Terminal too small. Resize to at least 64 x 18.",
                 columns);
        write_at(std::max(1, rows - 1), 0, "Esc: return", columns);
        refresh();
        return;
    }

    attron(COLOR_PAIR(1) | A_BOLD);
    mvhline(0, 0, ' ', columns);
    write_at(0, 2, "SUBSITE MONITOR SOFTWARE UPDATE", columns - 4);
    attroff(COLOR_PAIR(1) | A_BOLD);
    write_at(1, 2, "Installed version: " + std::string{current_version},
             columns - 4);
    write_at(2, 2, "Choose a newer verified release. Enter selects; Esc returns.",
             columns - 4);
    mvhline(3, 1, ACS_HLINE, columns - 2);

    const int list_rows = std::max(1, std::min(8, rows - 12));
    const std::size_t first_release =
        selected >= static_cast<std::size_t>(list_rows)
            ? selected - static_cast<std::size_t>(list_rows) + 1U
            : 0U;
    for (int line = 0; line < list_rows; ++line) {
        const std::size_t index = first_release + static_cast<std::size_t>(line);
        if (index >= releases.size()) break;
        const int row = 4 + line;
        const SoftwareRelease& release = releases[index];
        if (index == selected) {
            attron(COLOR_PAIR(2) | A_BOLD);
            mvhline(row, 1, ' ', columns - 2);
        }
        std::ostringstream entry;
        entry << release.version << "   " << release.published_date << "   "
              << safe_terminal_text(release.title);
        write_at(row, 2, entry.str(), columns - 4);
        if (index == selected) attroff(COLOR_PAIR(2) | A_BOLD);
    }

    const int notes_header_row = 4 + list_rows;
    mvhline(notes_header_row, 1, ACS_HLINE, columns - 2);
    if (!releases.empty()) {
        const SoftwareRelease& release = releases[selected];
        attron(COLOR_PAIR(3) | A_BOLD);
        write_at(notes_header_row + 1, 2, safe_terminal_text(release.title),
                 columns - 4);
        attroff(COLOR_PAIR(3) | A_BOLD);
        const std::vector<std::string> notes = release_note_lines(
            release.notes, static_cast<std::size_t>(columns - 4));
        const int notes_start = notes_header_row + 2;
        const int notes_capacity = std::max(0, rows - notes_start - 3);
        for (int line = 0; line < notes_capacity; ++line) {
            const std::size_t note_index =
                notes_offset + static_cast<std::size_t>(line);
            if (note_index >= notes.size()) break;
            write_at(notes_start + line, 2, notes[note_index], columns - 4);
        }
        if (notes_offset > 0U ||
            notes_offset + static_cast<std::size_t>(notes_capacity) < notes.size()) {
            write_at(rows - 3, 2, "n/p: scroll release notes", columns - 4);
        }
    }

    attron(COLOR_PAIR(1));
    write_at(rows - 2, 1,
             columns >= 69
                 ? " Select: Up/Dn | Review: Enter | Notes: n/p | Back: Esc | Cancel: q"
                 : "Select:Up/Dn | Review:Enter | Notes:n/p | Back:Esc | Cancel:q",
             columns - 2);
    attroff(COLOR_PAIR(1));
    if (!message.empty()) {
        attron(COLOR_PAIR(5) | A_BOLD);
        write_at(rows - 3, 2, safe_terminal_text(message), columns - 4);
        attroff(COLOR_PAIR(5) | A_BOLD);
    }
    refresh();
}

int ask_update_confirmation(const SoftwareRelease& release) {
    const int columns = getmaxx(stdscr);
    erase();
    attron(COLOR_PAIR(1) | A_BOLD);
    mvhline(0, 0, ' ', columns);
    write_at(0, 2, "CONFIRM SOFTWARE UPDATE", columns - 4);
    attroff(COLOR_PAIR(1) | A_BOLD);
    write_at(3, 2, "Install Subsite Monitor " + release.version + "?",
             columns - 4);
    write_at(5, 2, "This downloads the release and verifies its SHA-256.",
             columns - 4);
    write_at(6, 2, "The installer replaces the executable and may reload Nginx",
             columns - 4);
    write_at(7, 2, "and restart the Subsite Monitor systemd service.",
             columns - 4);
    write_at(9, 2, "Continue? Press y to install; any other key cancels.",
             columns - 4);
    refresh();
    const int key = getch();
    return key == 'y' || key == 'Y';
}

std::string update_from_ui() {
    erase();
    write_at(2, 2, "Checking GitHub for verified Subsite Monitor releases...",
             72);
    refresh();

    std::vector<SoftwareRelease> releases;
    try {
        releases = fetch_software_releases();
    } catch (const std::exception& error) {
        return error.what();
    }
    std::erase_if(releases, [](const SoftwareRelease& release) {
        return !version_is_newer(release.version, current_version);
    });
    if (releases.empty()) {
        return "No newer compatible release is available.";
    }

    std::size_t selected = 0;
    std::size_t notes_offset = 0;
    while (true) {
        draw_update_screen(releases, selected, notes_offset, {});
        const int key = getch();
        if (key == 27 || key == 'q' || key == 'Q') return {};
        if ((key == KEY_UP || key == 'k') && selected > 0U) {
            --selected;
            notes_offset = 0;
        } else if ((key == KEY_DOWN || key == 'j') &&
                   selected + 1U < releases.size()) {
            ++selected;
            notes_offset = 0;
        } else if (key == 'n') {
            const std::size_t note_count =
                release_note_lines(releases[selected].notes,
                                   static_cast<std::size_t>(
                                       std::max(1, getmaxx(stdscr) - 4)))
                    .size();
            if (notes_offset + 1U < note_count) ++notes_offset;
        } else if (key == 'p' && notes_offset > 0U) {
            --notes_offset;
        } else if (is_enter_key(key)) {
            if (!ask_update_confirmation(releases[selected])) {
                continue;
            }
            def_prog_mode();
            endwin();
            const int status = install_software_release(
                releases[selected].version, release_installer_path);
            reset_prog_mode();
            clear();
            refresh();
            if (status == 0) {
                return "Update installed. Quit and relaunch to use version " +
                       releases[selected].version + ".";
            }
            return "Update installer exited with status " +
                   std::to_string(status) + "; see the terminal output.";
        }
    }
}

bool is_enter_key(int key) {
    return key == '\n' || key == '\r' || key == KEY_ENTER;
}

} // namespace

int run_terminal_ui(const std::filesystem::path& log_path,
                    const std::filesystem::path& nginx_config,
                    const std::filesystem::path& statistics_directory) {
    if (!isatty(STDIN_FILENO) || !isatty(STDOUT_FILENO)) {
        throw std::runtime_error("ui requires an interactive terminal");
    }

    UiSnapshot snapshot =
        build_ui_statistics(log_path, nginx_config, statistics_directory);
    CursesSession session;
    configure_colors();
    const std::shared_ptr<UpdateCheckResult> update_check =
        start_update_check();
    timeout(250);
    std::size_t selected = 0;
    std::string status_message = std::move(snapshot.status_message);
    SiteSortColumn site_sort_column = SiteSortColumn::last_access;
    bool site_sort_ascending = false;
    bool history_mode = false;
    std::string history_hostname;
    RequestPage request_page;
    std::size_t request_selected = 0;
    std::size_t request_url_offset = 0;
    std::size_t request_total = 0;
    RequestSortColumn request_sort_column = RequestSortColumn::timestamp;
    bool request_sort_ascending = false;
    constexpr std::size_t request_page_size = 100U;

    while (true) {
        const std::vector<SiteRow> sites =
            sorted_sites(snapshot.report, site_sort_column,
                         site_sort_ascending);
        if (sites.empty()) {
            selected = 0;
        } else if (selected >= sites.size()) {
            selected = sites.size() - 1;
        }

        if (history_mode) {
            draw_request_screen(history_hostname, request_page,
                                request_total,
                                request_sort_column, request_sort_ascending,
                                request_selected, request_url_offset,
                                status_message);
        } else {
            draw_screen(snapshot.report, log_path, selected, site_sort_column,
                        site_sort_ascending, status_message);
        }
        int rows = 0;
        int columns = 0;
        getmaxyx(stdscr, rows, columns);
        draw_update_badge(rows, columns, update_check);
        refresh();
        status_message.clear();
        const int key = getch();
        if (key == ERR) {
            continue;
        }
        if (key == 'q' || key == 'Q' || key == 27) {
            if (history_mode && key == 27) {
                history_mode = false;
                continue;
            }
            break;
        }
        if (history_mode) {
            if (key >= '1' && key <= '4') {
                const auto selected_column = static_cast<RequestSortColumn>(
                    key - '1');
                if (selected_column == request_sort_column) {
                    request_sort_ascending = !request_sort_ascending;
                } else {
                    request_sort_column = selected_column;
                    request_sort_ascending = false;
                }
                sort_request_page(request_page, request_sort_column,
                                  request_sort_ascending);
                request_selected = 0;
                request_url_offset = 0;
                status_message = "Sorted this page " +
                    std::string{request_sort_ascending ? "ascending" :
                                                        "descending"};
            } else if (key == KEY_LEFT && request_url_offset > 0U) {
                request_url_offset = request_url_offset > 4U
                                         ? request_url_offset - 4U
                                         : 0U;
            } else if (key == KEY_RIGHT &&
                       request_selected < request_page.records.size()) {
                const std::size_t url_size =
                    request_page.records[request_selected].url.size();
                if (request_url_offset + 4U < url_size) {
                    request_url_offset += 4U;
                }
            } else if ((key == KEY_UP || key == 'k') &&
                       request_selected > 0U) {
                --request_selected;
                request_url_offset = 0;
            } else if ((key == KEY_DOWN || key == 'j') &&
                       request_selected + 1U < request_page.records.size()) {
                ++request_selected;
                request_url_offset = 0;
            } else if ((key == KEY_UP || key == 'k') &&
                       request_selected == 0U && request_page.has_more) {
                const std::size_t new_offset =
                    request_page.offset + request_page.records.size();
                request_page = read_newest_requests(
                    statistics_directory, history_hostname, new_offset,
                    request_page_size);
                sort_request_page(request_page, request_sort_column,
                                  request_sort_ascending);
                request_selected = request_page.records.empty()
                                       ? 0U
                                       : request_page.records.size() - 1U;
                request_url_offset = 0;
            } else if ((key == KEY_DOWN || key == 'j') &&
                       request_selected + 1U == request_page.records.size() &&
                       request_page.offset > 0U) {
                const std::size_t new_offset =
                    request_page.offset > request_page_size
                        ? request_page.offset - request_page_size
                        : 0U;
                request_page = read_newest_requests(
                    statistics_directory, history_hostname, new_offset,
                    request_page_size);
                sort_request_page(request_page, request_sort_column,
                                  request_sort_ascending);
                request_selected = 0;
                request_url_offset = 0;
            } else if (key == KEY_PPAGE && request_page.has_more) {
                const std::size_t new_offset =
                    request_page.offset + request_page.records.size();
                request_page = read_newest_requests(
                    statistics_directory, history_hostname, new_offset,
                    request_page_size);
                sort_request_page(request_page, request_sort_column,
                                  request_sort_ascending);
                request_selected = request_page.records.empty()
                                       ? 0U
                                       : request_page.records.size() - 1U;
                request_url_offset = 0;
            } else if (key == KEY_NPAGE && request_page.offset > 0U) {
                const std::size_t new_offset =
                    request_page.offset > request_page_size
                        ? request_page.offset - request_page_size
                        : 0U;
                request_page = read_newest_requests(
                    statistics_directory, history_hostname, new_offset,
                    request_page_size);
                sort_request_page(request_page, request_sort_column,
                                  request_sort_ascending);
                request_selected = 0;
                request_url_offset = 0;
            }
            continue;
        }
        if (key == 'u' || key == 'U') {
            status_message = update_from_ui();
        } else if (key >= '1' && key <= '3') {
            const auto selected_column =
                static_cast<SiteSortColumn>(key - '1');
            if (selected_column == site_sort_column) {
                site_sort_ascending = !site_sort_ascending;
            } else {
                site_sort_column = selected_column;
                site_sort_ascending =
                    selected_column == SiteSortColumn::hostname;
            }
            selected = 0;
            status_message = "Sorted sites " +
                std::string{site_sort_ascending ? "ascending" : "descending"};
        } else if ((key == KEY_UP || key == 'k') && selected > 0) {
            --selected;
        } else if ((key == KEY_DOWN || key == 'j') && selected + 1 < sites.size()) {
            ++selected;
        } else if (key == 'r' || key == 'R') {
            try {
                snapshot = build_ui_statistics(log_path, nginx_config,
                                               statistics_directory);
                if (snapshot.status_message.empty()) {
                    status_message =
                        "Log, CSV history, and Nginx site list refreshed.";
                } else {
                    status_message = std::move(snapshot.status_message);
                }
            } catch (const std::exception& error) {
                status_message = error.what();
            }
        } else if (is_enter_key(key) && !sites.empty()) {
            const SiteRow& site = sites[std::min(selected, sites.size() - 1)];
            history_hostname = std::string{site.hostname};
            try {
                request_total = count_site_requests(statistics_directory,
                                                    history_hostname);
                const std::size_t newest_page_offset =
                    request_total > request_page_size
                        ? request_total - request_page_size
                        : 0U;
                request_page = read_newest_requests(
                    statistics_directory, history_hostname,
                    newest_page_offset, request_page_size);
                request_sort_column = RequestSortColumn::timestamp;
                request_sort_ascending = false;
                sort_request_page(request_page, request_sort_column,
                                  request_sort_ascending);
                request_selected = 0;
                request_url_offset = 0;
                history_mode = true;
            } catch (const std::exception& error) {
                status_message = error.what();
            }
        }
    }
    return 0;
}

} // namespace subsite_monitor
