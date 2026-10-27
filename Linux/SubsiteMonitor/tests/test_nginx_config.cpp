#include "nginx_config.hpp"

#include <cassert>
#include <filesystem>
#include <stdexcept>
#include <string_view>
#include <vector>

int main() {
    const std::vector<std::string> names =
        subsite_monitor::discover_nginx_server_names(
            "tests/fixtures/nginx/nginx.conf");
    const std::vector<std::string> expected{
        "*.wild.example.test", "alpha.example.test", "beta.example.test",
        "root-alias.example.test", "root.example.test",
        "www.alpha.example.test"};
    assert(names == expected);

    try {
        (void)subsite_monitor::discover_nginx_server_names(
            "tests/fixtures/nginx/invalid.conf");
    } catch (const std::runtime_error& error) {
        assert(std::string_view{error.what()}.find("unclosed block") !=
               std::string_view::npos);
        return 0;
    }
    assert(false && "expected malformed Nginx config to be rejected");
}
