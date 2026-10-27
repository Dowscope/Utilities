#include <iostream>
#include <string_view>

namespace {

constexpr std::string_view version = "0.1.0";

void print_help(std::ostream& output) {
    output << "Usage: subsite-monitor [--help] [--version]\n"
              "Monitor website activity from web-server access logs.\n\n"
              "Options:\n"
              "  -h, --help       Show this help message\n"
              "  -V, --version    Show version information\n";
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

    std::cerr << "subsite-monitor: unrecognized or incomplete argument";
    if (!argument.empty()) {
        std::cerr << ": " << argument;
    }
    std::cerr << "\nTry 'subsite-monitor --help' for usage.\n";
    return 2;
}
