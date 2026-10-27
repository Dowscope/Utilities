#include "log_parser.hpp"

#include <cstdlib>
#include <iostream>
#include <string>
#include <string_view>

namespace {

void require(bool condition, std::string_view message) {
    if (!condition) {
        std::cerr << "Parser test failed: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

} // namespace

int main() {
    const auto first = subsite_monitor::parse_access_log_line(
        "2026-10-27T14:45:32+00:00 live.lakeheadschools.ca "
        "203.0.113.10 \"GET / HTTP/1.1\" 200");
    require(first.has_value(), "accept a valid request");
    require(first->timestamp == "2026-10-27T14:45:32+00:00",
            "extract timestamp");
    require(first->hostname == "live.lakeheadschools.ca",
            "extract virtual host");
    require(first->client_ip == "203.0.113.10", "extract client IP");
    require(first->method == "GET", "extract method");
    require(first->target == "/", "extract target");
    require(first->status == 200, "extract successful status");

    const auto second = subsite_monitor::parse_access_log_line(
        "2026-10-26T09:03:01-04:00 test.lakeheadschools.ca "
        "2001:db8::2 \"GET /private?token=hidden HTTP/2.0\" 404");
    require(second.has_value(), "accept another host, date, and HTTP/2 request");
    require(second->hostname == "test.lakeheadschools.ca",
            "extract second virtual host");
    require(second->target == "/private?token=hidden",
            "preserve the request target for internal parsing");
    require(second->status == 404, "extract HTTP error status");

    const auto escaped_quote = subsite_monitor::parse_access_log_line(
        "2026-10-27T14:45:32+00:00 example.test 192.0.2.1 "
        "\"GET /quoted\\\"path HTTP/1.1\" 500");
    require(escaped_quote.has_value(), "decode Nginx escaped quotes");
    require(escaped_quote->target == "/quoted\"path",
            "retain decoded escaped quote in target");

    require(!subsite_monitor::parse_access_log_line("").has_value(),
            "reject empty lines");
    require(!subsite_monitor::parse_access_log_line("not an access record")
                 .has_value(),
            "reject malformed lines");
    require(!subsite_monitor::parse_access_log_line(
                 "2026-02-30T14:45:32+00:00 example.test 192.0.2.1 "
                 "\"GET / HTTP/1.1\" 200")
                 .has_value(),
            "reject invalid dates");
    require(!subsite_monitor::parse_access_log_line(
                 "2026-10-27T14:45:32+00:00 bad host 192.0.2.1 "
                 "\"GET / HTTP/1.1\" 200")
                 .has_value(),
            "reject malformed host fields");
    require(!subsite_monitor::parse_access_log_line(
                 "2026-10-27T14:45:32+00:00 example.test 192.0.2.1 "
                 "\"GET / HTTP/1.1\" 600")
                 .has_value(),
            "reject out-of-range status");
    require(!subsite_monitor::parse_access_log_line(
                 "2026-10-27T14:45:32+00:00 example.test 192.0.2.1 "
                 "\"GET / HTTP/1.1\" 200 trailing")
                 .has_value(),
            "reject trailing fields outside the supported format");
    require(!subsite_monitor::parse_access_log_line(
                 "2026-10-27T14:45:32+00:00 example.test 192.0.2.1 "
                 "\"" + std::string(33, 'A') + " / HTTP/1.1\" 200")
                 .has_value(),
            "reject methods above the field size limit");
    const std::string long_line(16385, 'x');
    require(!subsite_monitor::parse_access_log_line(long_line).has_value(),
            "reject lines above the parser size limit");
    const std::string long_target_line =
        "2026-10-27T14:45:32+00:00 example.test 192.0.2.1 \"GET /" +
        std::string(8192, 'a') + " HTTP/1.1\" 200";
    require(!subsite_monitor::parse_access_log_line(long_target_line).has_value(),
            "reject targets above the field size limit");

    std::cout << "Parser tests passed\n";
}
