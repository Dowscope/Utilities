#include "collector.hpp"
#include "report.hpp"
#include "statistics.hpp"
#include "tui.hpp"

#include <exception>
#include <iostream>
#include <string_view>

namespace {

#ifndef SUBSITE_MONITOR_VERSION
#define SUBSITE_MONITOR_VERSION "0.1.0-alpha.1"
#endif

constexpr std::string_view version = SUBSITE_MONITOR_VERSION;

void print_help(std::ostream& output) {
    output << "Usage:\n"
              "  subsite-monitor --help\n"
              "  subsite-monitor --version\n"
              "  subsite-monitor collect --log <path>\n"
              "  subsite-monitor stats --log <path>\n"
              "  subsite-monitor ui --log <path>\n\n"
              "Monitor website activity from web-server access logs.\n\n"
              "Options:\n"
              "  -h, --help       Show this help message\n"
              "  -V, --version    Show version information\n\n"
              "The collect command accepts the subsite_monitor Nginx\n"
              "access-log format described in the project manual.\n"
              "The ui command requires an interactive terminal.\n";
}

} // namespace

int main(int argc, char* argv[]) {
    if (argc == 1) {
        print_help(std::cout);
        return 0;
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
        if (argc != 4 || std::string_view{argv[2]} != "--log" ||
            std::string_view{argv[3]}.empty()) {
            std::cerr << "subsite-monitor: ui requires --log <path>\n"
                         "Try 'subsite-monitor --help' for usage.\n";
            return 2;
        }
        try {
            return subsite_monitor::run_terminal_ui(argv[3]);
        } catch (const std::exception& error) {
            std::cerr << "subsite-monitor: " << error.what() << '\n';
            return 1;
        }
    }

    std::cerr << "subsite-monitor: unrecognized or incomplete argument";
    if (!argument.empty()) {
        std::cerr << ": " << argument;
    }
    std::cerr << "\nTry 'subsite-monitor --help' for usage.\n";
    return 2;
}
