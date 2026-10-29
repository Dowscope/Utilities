#include "config.hpp"
#include "collector.hpp"
#include "nginx_config.hpp"
#include "report.hpp"
#include "statistics.hpp"
#include "storage.hpp"
#include "service.hpp"
#include "tui.hpp"

#include <charconv>
#include <cstdlib>
#include <exception>
#include <filesystem>
#include <iostream>
#include <optional>
#include <span>
#include <string_view>
#include <utility>

namespace {

#ifndef SUBSITE_MONITOR_VERSION
#define SUBSITE_MONITOR_VERSION "0.1.0-alpha.5"
#endif

constexpr std::string_view version = SUBSITE_MONITOR_VERSION;
constexpr std::string_view default_config_path =
    "/etc/subsite-monitor/monitor.conf";

struct UiSettings {
    std::filesystem::path access_log;
    std::filesystem::path nginx_config;
    std::filesystem::path statistics_directory;
};

std::filesystem::path configured_statistics_directory() {
    const char* environment_directory =
        std::getenv("SUBSITE_MONITOR_STATS_DIR");
    if (environment_directory != nullptr && environment_directory[0] != '\0') {
        return environment_directory;
    }
    return subsite_monitor::default_statistics_directory();
}

UiSettings configured_ui_settings() {
    const char* environment_log = std::getenv("SUBSITE_MONITOR_LOG");
    if (environment_log != nullptr && environment_log[0] != '\0') {
        const char* environment_nginx_config =
            std::getenv("SUBSITE_MONITOR_NGINX_CONFIG");
        return {environment_log,
                environment_nginx_config != nullptr &&
                        environment_nginx_config[0] != '\0'
                    ? std::filesystem::path{environment_nginx_config}
                    : std::filesystem::path{},
                configured_statistics_directory()};
    }

    const char* environment_config = std::getenv("SUBSITE_MONITOR_CONFIG");
    const std::filesystem::path config_path =
        environment_config != nullptr && environment_config[0] != '\0'
            ? std::filesystem::path{environment_config}
            : std::filesystem::path{default_config_path};
    subsite_monitor::MonitorConfig config =
        subsite_monitor::load_config(config_path);
    const char* environment_nginx_config =
        std::getenv("SUBSITE_MONITOR_NGINX_CONFIG");
    if (environment_nginx_config != nullptr &&
        environment_nginx_config[0] != '\0') {
        config.nginx_config = environment_nginx_config;
    }
    return {std::move(config.access_log), std::move(config.nginx_config),
            configured_statistics_directory()};
}

int launch_ui(const std::filesystem::path& log_path,
              const std::filesystem::path& nginx_config,
              const std::filesystem::path& statistics_directory,
              std::span<char* const> startup_arguments) {
    try {
        return subsite_monitor::run_terminal_ui(log_path, nginx_config,
                                                statistics_directory,
                                                startup_arguments);
    } catch (const std::exception& error) {
        std::cerr << "subsite-monitor: " << error.what() << '\n';
        return 1;
    }
}

int launch_default_ui(int argc, char* argv[]) {
    try {
        const UiSettings settings = configured_ui_settings();
        return launch_ui(settings.access_log, settings.nginx_config,
                         settings.statistics_directory,
                         {argv, static_cast<std::size_t>(argc)});
    } catch (const std::exception& error) {
        std::cerr << "subsite-monitor: " << error.what() << '\n';
        return 1;
    }
}

void print_help(std::ostream& output) {
    output << "Usage:\n"
              "  subsite-monitor\n"
              "  subsite-monitor --help\n"
              "  subsite-monitor --version\n"
              "  subsite-monitor collect --log <path> [--stats-dir <path>]\n"
              "  subsite-monitor stats [--days <count>] [--stats-dir <path>]\n"
              "  subsite-monitor stats --log <path>\n"
              "  subsite-monitor service\n"
              "  subsite-monitor ui --log <path>\n\n"
              "Monitor website activity from web-server access logs.\n\n"
              "Options:\n"
              "  -h, --help       Show this help message\n"
              "  -V, --version    Show version information\n\n"
              "With no arguments, open the UI using the root-only\n"
              "/etc/subsite-monitor/monitor.conf configuration. The ui\n"
              "command uses the same configuration unless --log is supplied.\n"
              "The UI reads Nginx server_name values from nginx_config.\n"
              "Environment overrides: SUBSITE_MONITOR_CONFIG,\n"
              "SUBSITE_MONITOR_LOG, and SUBSITE_MONITOR_NGINX_CONFIG.\n\n"
              "collect scans the supported host-aware Nginx log and replaces\n"
              "daily summaries and per-request event CSV snapshots.\n"
              "stats reads stored CSV data; --days selects the most recent\n"
              "stored dates. --log prints a one-time log report.\n"
              "Environment overrides: SUBSITE_MONITOR_STATS_DIR.\n"
              "service follows the configured log and updates CSV data.\n"
              "The ui command requires an interactive terminal.\n\n"
              "Issues and support: https://github.com/Dowscope/Utilities/issues\n";
}

int launch_service() {
    try {
        const UiSettings settings = configured_ui_settings();
        return subsite_monitor::run_monitor_service(
            settings.access_log, settings.statistics_directory);
    } catch (const std::exception& error) {
        std::cerr << "subsite-monitor: " << error.what() << '\n';
        return 1;
    }
}

std::optional<std::size_t> parse_positive_size(std::string_view value) {
    std::size_t parsed = 0;
    const char* first = value.data();
    const char* last = first + value.size();
    const auto [end, error] = std::from_chars(first, last, parsed);
    if (error != std::errc{} || end != last || parsed == 0 || parsed > 10000U) {
        return std::nullopt;
    }
    return parsed;
}

int run_collect(int argc, char* argv[]) {
    std::optional<std::filesystem::path> log_path;
    std::filesystem::path statistics_directory =
        configured_statistics_directory();
    bool statistics_directory_seen = false;
    for (int index = 2; index < argc;) {
        const std::string_view option{argv[index]};
        if ((option == "--log" || option == "--stats-dir") &&
            index + 1 < argc && std::string_view{argv[index + 1]}.size() > 0) {
            if (option == "--log") {
                if (log_path) {
                    std::cerr << "subsite-monitor: collect repeats --log\n";
                    return 2;
                }
                log_path = argv[index + 1];
            } else {
                if (statistics_directory_seen) {
                    std::cerr << "subsite-monitor: collect repeats --stats-dir\n";
                    return 2;
                }
                statistics_directory = argv[index + 1];
                statistics_directory_seen = true;
            }
            index += 2;
        } else {
            std::cerr << "subsite-monitor: collect accepts --log <path> and "
                         "optional --stats-dir <path>\n"
                         "Try 'subsite-monitor --help' for usage.\n";
            return 2;
        }
    }
    if (!log_path) {
        std::cerr << "subsite-monitor: collect requires --log <path>\n"
                     "Try 'subsite-monitor --help' for usage.\n";
        return 2;
    }
    try {
        const subsite_monitor::StatisticsReport report =
            subsite_monitor::collect_and_store(*log_path,
                                               statistics_directory);
        std::uint64_t parsed_records = 0;
        for (const auto& [hostname, statistics] : report.sites) {
            (void)hostname;
            parsed_records += statistics.requests;
        }
        std::cout << "Parsed records: " << parsed_records << '\n'
                  << "Malformed lines: " << report.malformed_lines << '\n'
                  << "Daily CSV files updated in: "
                  << statistics_directory.string() << '\n';
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "subsite-monitor: " << error.what() << '\n';
        return 1;
    }
}

int run_stats(int argc, char* argv[]) {
    std::optional<std::filesystem::path> log_path;
    std::optional<std::size_t> days;
    std::filesystem::path statistics_directory =
        configured_statistics_directory();
    bool statistics_directory_seen = false;
    for (int index = 2; index < argc;) {
        const std::string_view option{argv[index]};
        if (option == "--log" && index + 1 < argc &&
            std::string_view{argv[index + 1]}.size() > 0) {
            if (log_path) {
                std::cerr << "subsite-monitor: stats repeats --log\n";
                return 2;
            }
            log_path = argv[index + 1];
            index += 2;
        } else if (option == "--days" && index + 1 < argc) {
            if (days) {
                std::cerr << "subsite-monitor: stats repeats --days\n";
                return 2;
            }
            days = parse_positive_size(argv[index + 1]);
            if (!days) {
                std::cerr << "subsite-monitor: --days must be an integer from 1 to 10000\n";
                return 2;
            }
            index += 2;
        } else if (option == "--stats-dir" && index + 1 < argc &&
                   std::string_view{argv[index + 1]}.size() > 0) {
            if (statistics_directory_seen) {
                std::cerr << "subsite-monitor: stats repeats --stats-dir\n";
                return 2;
            }
            statistics_directory = argv[index + 1];
            statistics_directory_seen = true;
            index += 2;
        } else {
            std::cerr << "subsite-monitor: stats accepts --log <path>, "
                         "--days <count>, and --stats-dir <path>\n"
                         "Try 'subsite-monitor --help' for usage.\n";
            return 2;
        }
    }
    if (log_path && (days || statistics_directory_seen)) {
        std::cerr << "subsite-monitor: --log cannot be combined with stored-statistics options\n";
        return 2;
    }
    try {
        const subsite_monitor::StatisticsReport report =
            log_path ? subsite_monitor::build_statistics(*log_path)
                     : subsite_monitor::read_stored_statistics(
                           statistics_directory, days.value_or(0));
        subsite_monitor::print_statistics(report, std::cout);
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "subsite-monitor: " << error.what() << '\n';
        return 1;
    }
}

} // namespace

int main(int argc, char* argv[]) {
    if (argc == 1) {
        return launch_default_ui(argc, argv);
    }

    const std::string_view argument{argv[1]};
    if (argc == 2 && (argument == "--help" || argument == "-h")) {
        print_help(std::cout);
        return 0;
    }
    if (argc == 2 && (argument == "--version" || argument == "-V")) {
        std::cout << "subsite-monitor " << version << '\n';
        return 0;
    }

    if (argument == "collect") {
        return run_collect(argc, argv);
    }

    if (argument == "stats") {
        return run_stats(argc, argv);
    }

    if (argument == "service") {
        if (argc != 2) {
            std::cerr << "subsite-monitor: service accepts no options\n"
                         "Try 'subsite-monitor --help' for usage.\n";
            return 2;
        }
        return launch_service();
    }

    if (argument == "ui") {
        if (argc == 2) {
            return launch_default_ui(argc, argv);
        }
        if (argc != 4 || std::string_view{argv[2]} != "--log" ||
            std::string_view{argv[3]}.empty()) {
            std::cerr << "subsite-monitor: ui accepts no options or --log <path>\n"
                         "Try 'subsite-monitor --help' for usage.\n";
            return 2;
        }
        const char* environment_nginx_config =
            std::getenv("SUBSITE_MONITOR_NGINX_CONFIG");
        const std::filesystem::path nginx_config =
            environment_nginx_config != nullptr &&
                    environment_nginx_config[0] != '\0'
                ? std::filesystem::path{environment_nginx_config}
                : std::filesystem::path{};
        return launch_ui(argv[3], nginx_config,
                         configured_statistics_directory(),
                         {argv, static_cast<std::size_t>(argc)});
    }

    std::cerr << "subsite-monitor: unrecognized or incomplete argument";
    if (!argument.empty()) {
        std::cerr << ": " << argument;
    }
    std::cerr << "\nTry 'subsite-monitor --help' for usage.\n";
    return 2;
}
