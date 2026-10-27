#include "tui.hpp"

#include "report.hpp"
#include "statistics.hpp"

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
    write_at(5, last_access_column, "LAST ACCESS (UTC)", columns - last_access_column - 2);
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
        write_at(row, last_access_column,
                 format_timestamp_utc(site.statistics->latest_access_utc),
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

    mvhline(rows - 2, 0, ' ', columns);
    attron(COLOR_PAIR(1));
    const std::string footer = " UP/DOWN or j/k  Select site     r  Refresh     q  Quit ";
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

} // namespace

int run_terminal_ui(const std::filesystem::path& log_path) {
    if (!isatty(STDIN_FILENO) || !isatty(STDOUT_FILENO)) {
        throw std::runtime_error("ui requires an interactive terminal");
    }

    StatisticsReport report = build_statistics(log_path);
    CursesSession session;
    configure_colors();
    std::size_t selected = 0;
    std::string status_message;

    while (true) {
        const std::vector<SiteRow> sites = sorted_sites(report);
        if (sites.empty()) {
            selected = 0;
        } else if (selected >= sites.size()) {
            selected = sites.size() - 1;
        }

        draw_screen(report, log_path, selected, status_message);
        status_message.clear();
        const int key = getch();
        if (key == 'q' || key == 'Q' || key == 27) {
            break;
        }
        if ((key == KEY_UP || key == 'k') && selected > 0) {
            --selected;
        } else if ((key == KEY_DOWN || key == 'j') && selected + 1 < sites.size()) {
            ++selected;
        } else if (key == 'r' || key == 'R') {
            try {
                report = build_statistics(log_path);
                status_message = "Statistics refreshed from access log.";
            } catch (const std::exception& error) {
                status_message = error.what();
            }
        }
    }
    return 0;
}

} // namespace subsite_monitor
