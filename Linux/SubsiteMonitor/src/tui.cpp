#include "tui.hpp"

#include "report.hpp"
#include "nginx_config.hpp"
#include "statistics.hpp"
#include "storage.hpp"

#include <algorithm>
#include <exception>
#include <filesystem>
#include <sstream>
#include <string>
#include <string_view>
#include <stdexcept>
#include <utility>
#include <vector>

#include <ncurses.h>
#include <unistd.h>

namespace subsite_monitor {
namespace {

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

std::vector<SiteRow> sorted_sites(const StatisticsReport& report) {
    std::vector<SiteRow> sites;
    sites.reserve(report.sites.size());
    for (const auto& [hostname, statistics] : report.sites) {
        sites.push_back({hostname, &statistics});
    }
    std::sort(sites.begin(), sites.end(), [](const SiteRow& left,
                                             const SiteRow& right) {
        if (left.statistics->requests != right.statistics->requests) {
            return left.statistics->requests > right.statistics->requests;
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

void configure_colors() {
    if (has_colors() == false || start_color() == ERR) {
        return;
    }
    use_default_colors();
    init_pair(1, COLOR_WHITE, COLOR_BLUE);
    init_pair(2, COLOR_BLACK, COLOR_CYAN);
    init_pair(3, COLOR_CYAN, -1);
    init_pair(4, COLOR_GREEN, -1);
    init_pair(5, COLOR_YELLOW, -1);
}

void draw_screen(const StatisticsReport& report,
                 const std::filesystem::path& log_path, std::size_t selected,
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

    const std::vector<SiteRow> sites = sorted_sites(report);
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
    write_at(5, 2, "WEBSITE", site_width);
    write_at(5, requests_column, "REQUESTS", 10);
    write_at(5, last_access_column, "LAST ACCESS (SERVER TIME)", columns - last_access_column - 2);
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
        } else if (visible_row % 2 == 1) {
            attron(A_DIM);
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
        } else if (visible_row % 2 == 1) {
            attroff(A_DIM);
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
    const std::string footer =
        " UP/DOWN or j/k  Select     Enter  Request data     r  Refresh     q  Quit ";
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
              << page.offset + page.records.size();
        if (page.has_more) {
            range << "+";
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
    write_at(3, 2, "SERVER TIME", time_width);
    write_at(3, ip_column, "CLIENT IP", ip_width);
    write_at(3, method_column, "METHOD/STATUS", method_width);
    write_at(3, url_column, "URL", url_width);
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
            } else if (line % 2U == 1U) {
                attron(A_DIM);
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
            } else if (line % 2U == 1U) {
                attroff(A_DIM);
            }
        }
    }

    mvhline(rows - 2, 0, ' ', columns);
    attron(COLOR_PAIR(1));
    std::ostringstream footer;
    footer << " UP/DN Move  LEFT/RIGHT URL  PGUP/PGDN Page  ESC Back  q Quit";
    write_at(rows - 2, 1, footer.str(), columns - 2);
    attroff(COLOR_PAIR(1));
    if (!status_message.empty()) {
        attron(COLOR_PAIR(5) | A_BOLD);
        write_at(rows - 3, 2, safe_terminal_text(status_message), content_width);
        attroff(COLOR_PAIR(5) | A_BOLD);
    }
    refresh();
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
    std::size_t selected = 0;
    std::string status_message = std::move(snapshot.status_message);
    bool history_mode = false;
    std::string history_hostname;
    RequestPage request_page;
    std::size_t request_selected = 0;
    std::size_t request_url_offset = 0;
    constexpr std::size_t request_page_size = 100U;

    while (true) {
        const std::vector<SiteRow> sites = sorted_sites(snapshot.report);
        if (sites.empty()) {
            selected = 0;
        } else if (selected >= sites.size()) {
            selected = sites.size() - 1;
        }

        if (history_mode) {
            draw_request_screen(history_hostname, request_page,
                                request_selected, request_url_offset,
                                status_message);
        } else {
            draw_screen(snapshot.report, log_path, selected, status_message);
        }
        status_message.clear();
        const int key = getch();
        if (key == 'q' || key == 'Q' || key == 27) {
            if (history_mode && key == 27) {
                history_mode = false;
                continue;
            }
            break;
        }
        if (history_mode) {
            if (key == KEY_LEFT && request_url_offset > 0U) {
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
                       request_selected == 0U && request_page.offset > 0U) {
                const std::size_t new_offset =
                    request_page.offset > request_page_size
                        ? request_page.offset - request_page_size
                        : 0U;
                request_page = read_site_requests(
                    statistics_directory, history_hostname, new_offset,
                    request_page_size);
                request_selected = request_page.records.empty()
                                       ? 0U
                                       : request_page.records.size() - 1U;
                request_url_offset = 0;
            } else if ((key == KEY_DOWN || key == 'j') &&
                       request_selected + 1U == request_page.records.size() &&
                       request_page.has_more) {
                const std::size_t new_offset =
                    request_page.offset + request_page.records.size();
                request_page = read_site_requests(
                    statistics_directory, history_hostname, new_offset,
                    request_page_size);
                request_selected = 0;
                request_url_offset = 0;
            } else if (key == KEY_PPAGE && request_page.offset > 0U) {
                const std::size_t new_offset =
                    request_page.offset > request_page_size
                        ? request_page.offset - request_page_size
                        : 0U;
                request_page = read_site_requests(
                    statistics_directory, history_hostname, new_offset,
                    request_page_size);
                request_selected = 0;
                request_url_offset = 0;
            } else if (key == KEY_NPAGE && request_page.has_more) {
                const std::size_t new_offset =
                    request_page.offset + request_page.records.size();
                request_page = read_site_requests(
                    statistics_directory, history_hostname, new_offset,
                    request_page_size);
                request_selected = 0;
                request_url_offset = 0;
            }
            continue;
        }
        if ((key == KEY_UP || key == 'k') && selected > 0) {
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
                request_page = read_site_requests(
                    statistics_directory, history_hostname, 0,
                    request_page_size);
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
