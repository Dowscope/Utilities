#include "config.hpp"
#include "collector.hpp"
#include "nginx_config.hpp"
#include "report.hpp"
#include "statistics.hpp"
#include "tui.hpp"

#include <cstdlib>
#include <exception>
#include <filesystem>
#include <iostream>
#include <string_view>
#include <utility>

namespace {

#ifndef SUBSITE_MONITOR_VERSION
#define SUBSITE_MONITOR_VERSION "0.1.0-alpha.3"
#endif

constexpr std::string_view version = SUBSITE_MONITOR_VERSION;
constexpr std::string_view default_config_path =
    "/etc/subsite-monitor/monitor.conf";

struct UiSettings {
    std::filesystem::path access_log;
    std::filesystem::path nginx_config;
};

UiSettings configured_ui_settings() {
    const char* environment_log = std::getenv("SUBSITE_MONITOR_LOG");
    if (environment_log != nullptr && environment_log[0] != '\0') {
        const char* environment_nginx_config =
            std::getenv("SUBSITE_MONITOR_NGINX_CONFIG");
        return {environment_log,
                environment_nginx_config != nullptr &&
                        environment_nginx_config[0] != '\0'
                    ? std::filesystem::path{environment_nginx_config}
                    : std::filesystem::path{}};
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
    return {std::move(config.access_log), std::move(config.nginx_config)};
}

int launch_ui(const std::filesystem::path& log_path,
              const std::filesystem::path& nginx_config = {}) {
    try {
        return subsite_monitor::run_terminal_ui(log_path, nginx_config);
    } catch (const std::exception& error) {
        std::cerr << "subsite-monitor: " << error.what() << '\n';
        return 1;
    }
}

int launch_default_ui() {
    try {
        const UiSettings settings = configured_ui_settings();
        return launch_ui(settings.access_log, settings.nginx_config);
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
              "  subsite-monitor collect --log <path>\n"
              "  subsite-monitor stats --log <path>\n"
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
              "The collect command accepts the subsite_monitor Nginx\n"
              "access-log format described in the project manual.\n"
              "The ui command requires an interactive terminal.\n";
}

} // namespace

int main(int argc, char* argv[]) {
    if (argc == 1) {
        return launch_default_ui();
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
        if (argc != 4 || std::string_view{argv[2]} != "--log" ||
            std::string_view{argv[3]}.empty()) {
            std::cerr << "subsite-monitor: collect requires --log <path>\n"
                         "Try 'subsite-monitor --help' for usage.\n";
            return 2;
        }
        try {
            const subsite_monitor::CollectionSummary summary =
                subsite_monitor::collect_log(argv[3]);
            std::cout << "Parsed records: " << summary.parsed_records << '\n'
                      << "Malformed lines: " << summary.malformed_lines
                      << '\n';
            return 0;
        } catch (const std::exception& error) {
            std::cerr << "subsite-monitor: " << error.what() << '\n';
            return 1;
        }
    }

    if (argument == "stats") {
        if (argc != 4 || std::string_view{argv[2]} != "--log" ||
            std::string_view{argv[3]}.empty()) {
            std::cerr << "subsite-monitor: stats currently requires --log <path>\n"
                         "Try 'subsite-monitor --help' for usage.\n";
            return 2;
        }
        try {
            const subsite_monitor::StatisticsReport report =
                subsite_monitor::build_statistics(argv[3]);
            subsite_monitor::print_statistics(report, std::cout);
            return 0;
        } catch (const std::exception& error) {
            std::cerr << "subsite-monitor: " << error.what() << '\n';
            return 1;
        }
    }

    if (argument == "ui") {
        if (argc == 2) {
            return launch_default_ui();
        }
        if (argc != 4 || std::string_view{argv[2]} != "--log" ||
            std::string_view{argv[3]}.empty()) {
            std::cerr << "subsite-monitor: ui accepts no options or --log <path>\n"
                         "Try 'subsite-monitor --help' for usage.\n";
            return 2;
        }
        return launch_ui(argv[3]);
    }

    std::cerr << "subsite-monitor: unrecognized or incomplete argument";
    if (!argument.empty()) {
        std::cerr << ": " << argument;
    }
    std::cerr << "\nTry 'subsite-monitor --help' for usage.\n";
    return 2;
}
