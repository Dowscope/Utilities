#include "nginx_config.hpp"

#include <algorithm>
#include <array>
#include <cctype>
#include <cstddef>
#include <fstream>
#include <set>
#include <stdexcept>
#include <string>
#include <string_view>
#include <system_error>
#include <utility>
#include <vector>

namespace subsite_monitor {
namespace {

constexpr std::size_t max_file_size = 256U * 1024U;
constexpr std::size_t max_total_size = 8U * 1024U * 1024U;
constexpr std::size_t max_tokens_per_file = 32768U;
constexpr std::size_t max_files = 1024U;
constexpr std::size_t max_file_parses = 4096U;
constexpr std::size_t max_include_depth = 32U;
constexpr std::size_t max_block_depth = 64U;
constexpr std::size_t max_token_size = 4096U;
constexpr std::size_t max_sites = 10000U;

bool is_space(char value) {
    const auto byte = static_cast<unsigned char>(value);
    return std::isspace(byte) != 0;
}

std::vector<std::string> tokenize(std::string_view input) {
    std::vector<std::string> tokens;
    std::size_t position = 0;
    while (position < input.size()) {
        const char current = input[position];
        if (is_space(current)) {
            ++position;
            continue;
        }
        if (current == '#') {
            while (position < input.size() && input[position] != '\n') {
                ++position;
            }
            continue;
        }
        if (current == ';' || current == '{' || current == '}') {
            tokens.emplace_back(1U, current);
            ++position;
        } else if (current == '\'' || current == '"') {
            const char quote = current;
            ++position;
            std::string token;
            bool closed = false;
            while (position < input.size()) {
                const char value = input[position++];
                if (value == quote) {
                    closed = true;
                    break;
                }
                if (value == '\\') {
                    if (position == input.size()) {
                        throw std::runtime_error(
                            "Nginx config ends with an incomplete escape");
                    }
                    token.push_back(input[position++]);
                } else {
                    token.push_back(value);
                }
                if (token.size() > max_token_size) {
                    throw std::runtime_error(
                        "Nginx config token exceeds the size limit");
                }
            }
            if (!closed) {
                throw std::runtime_error("Nginx config has an unclosed quote");
            }
            tokens.push_back(std::move(token));
        } else {
            std::string token;
            while (position < input.size()) {
                const char value = input[position];
                if (is_space(value) || value == ';' || value == '{' ||
                    value == '}' || value == '#') {
                    break;
                }
                ++position;
                if (value == '\\') {
                    if (position == input.size()) {
                        throw std::runtime_error(
                            "Nginx config ends with an incomplete escape");
                    }
                    token.push_back(input[position++]);
                } else {
                    token.push_back(value);
                }
                if (token.size() > max_token_size) {
                    throw std::runtime_error(
                        "Nginx config token exceeds the size limit");
                }
            }
            if (token.empty()) {
                throw std::runtime_error("Nginx config has an invalid token");
            }
            tokens.push_back(std::move(token));
        }
        if (tokens.size() > max_tokens_per_file) {
            throw std::runtime_error("Nginx config has too many tokens");
        }
    }
    return tokens;
}

bool has_wildcard(std::string_view value) {
    return value.find_first_of("*?") != std::string_view::npos;
}

bool wildcard_matches(std::string_view pattern, std::string_view value) {
    std::size_t pattern_position = 0;
    std::size_t value_position = 0;
    std::size_t star_position = std::string_view::npos;
    std::size_t retry_position = 0;

    while (value_position < value.size()) {
        if (pattern_position < pattern.size() &&
            (pattern[pattern_position] == '?' ||
             pattern[pattern_position] == value[value_position])) {
            ++pattern_position;
            ++value_position;
        } else if (pattern_position < pattern.size() &&
                   pattern[pattern_position] == '*') {
            star_position = pattern_position++;
            retry_position = value_position;
        } else if (star_position != std::string_view::npos) {
            pattern_position = star_position + 1U;
            value_position = ++retry_position;
        } else {
            return false;
        }
    }
    while (pattern_position < pattern.size() &&
           pattern[pattern_position] == '*') {
        ++pattern_position;
    }
    return pattern_position == pattern.size();
}

class NginxConfigScanner {
public:
    explicit NginxConfigScanner(std::filesystem::path main_config)
        : root_directory_(std::move(main_config).parent_path()) {
        if (root_directory_.empty()) {
            root_directory_ = ".";
        }
    }

    std::vector<std::string> scan(const std::filesystem::path& main_config) {
        parse_file(main_config, {}, 0U);
        return {server_names_.begin(), server_names_.end()};
    }

private:
    std::filesystem::path root_directory_;
    std::set<std::filesystem::path> parsed_files_;
    std::set<std::filesystem::path> active_files_;
    std::set<std::string> server_names_;
    std::size_t total_size_{};
    std::size_t file_parse_count_{};

    static std::string read_file(const std::filesystem::path& path,
                                 std::size_t expected_size) {
        std::ifstream input(path, std::ios::binary);
        if (!input) {
            throw std::runtime_error("cannot open Nginx config: " +
                                     path.string());
        }
        std::string contents;
        contents.reserve(expected_size);
        std::array<char, 8192U> buffer{};
        while (input) {
            input.read(buffer.data(),
                       static_cast<std::streamsize>(buffer.size()));
            const std::streamsize count = input.gcount();
            if (count > 0) {
                const auto amount = static_cast<std::size_t>(count);
                if (amount > max_file_size - contents.size()) {
                    throw std::runtime_error(
                        "Nginx config exceeds the per-file size limit: " +
                        path.string());
                }
                contents.append(buffer.data(), amount);
            }
        }
        if (input.bad()) {
            throw std::runtime_error("cannot read Nginx config: " +
                                     path.string());
        }
        return contents;
    }

    std::vector<std::filesystem::path> resolve_include(
        std::string_view value) const {
        if (value.empty() || value.find_first_of("$\\\r\n\t") !=
                                 std::string_view::npos) {
            throw std::runtime_error("Nginx include path is empty or dynamic");
        }
        std::filesystem::path pattern{value};
        if (!pattern.is_absolute()) {
            pattern = root_directory_ / pattern;
        }

        const std::filesystem::path directory =
            pattern.parent_path().empty() ? root_directory_
                                          : pattern.parent_path();
        const std::string filename_pattern = pattern.filename().string();
        if (filename_pattern.empty()) {
            throw std::runtime_error("Nginx include path must name a file");
        }
        if (!has_wildcard(pattern.parent_path().string())) {
            if (!has_wildcard(filename_pattern)) {
                if (!std::filesystem::is_regular_file(pattern)) {
                    throw std::runtime_error("included Nginx config is missing: " +
                                             pattern.string());
                }
                return {pattern};
            }

            std::vector<std::filesystem::path> matches;
            std::error_code error;
            for (std::filesystem::directory_iterator iterator{directory, error},
                 end;
                 !error && iterator != end; iterator.increment(error)) {
                if (!iterator->is_regular_file(error) || error) {
                    error.clear();
                    continue;
                }
                if (wildcard_matches(filename_pattern,
                                     iterator->path().filename().string())) {
                    matches.push_back(iterator->path());
                }
            }
            if (error) {
                throw std::runtime_error("cannot scan Nginx include directory: " +
                                         directory.string());
            }
            if (matches.size() > max_files) {
                throw std::runtime_error(
                    "Nginx include pattern matches too many files");
            }
            std::sort(matches.begin(), matches.end());
            return matches;
        }
        throw std::runtime_error(
            "Nginx include wildcards are supported only in the filename");
    }

    void add_server_names(const std::vector<std::string>& arguments) {
        for (const std::string& name : arguments) {
            if (name.empty() || name == "_" || name.front() == '~' ||
                name.front() == '$' || name.size() > 253U) {
                continue;
            }
            bool valid = true;
            for (std::size_t position = 0; position < name.size(); ++position) {
                const auto byte =
                    static_cast<unsigned char>(name[position]);
                const bool ordinary =
                    (byte >= 'a' && byte <= 'z') ||
                    (byte >= 'A' && byte <= 'Z') ||
                    (byte >= '0' && byte <= '9') || name[position] == '.' ||
                    name[position] == '-' || name[position] == '_';
                const bool wildcard = name[position] == '*' &&
                    (position == 0U || position + 1U == name.size());
                if (!ordinary && !wildcard) {
                    valid = false;
                    break;
                }
            }
            if (!valid || name == "*") {
                continue;
            }
            std::string normalized = name;
            std::transform(normalized.begin(), normalized.end(),
                           normalized.begin(), [](char value) {
                               if (value >= 'A' && value <= 'Z') {
                                   return static_cast<char>(value - 'A' + 'a');
                               }
                               return value;
                           });
            server_names_.insert(std::move(normalized));
            if (server_names_.size() > max_sites) {
                throw std::runtime_error(
                    "Nginx config has too many server names");
            }
        }
    }

    void parse_sequence(const std::vector<std::string>& tokens,
                        std::size_t& position,
                        std::vector<std::string>& contexts, bool expects_close,
                        std::size_t block_depth, std::size_t include_depth) {
        while (position < tokens.size()) {
            if (tokens[position] == "}") {
                if (!expects_close) {
                    throw std::runtime_error(
                        "Nginx config has an unmatched closing brace");
                }
                ++position;
                return;
            }
            if (tokens[position] == ";" || tokens[position] == "{") {
                throw std::runtime_error("Nginx config has an empty directive");
            }

            const std::string directive = tokens[position++];
            std::vector<std::string> arguments;
            bool complete = false;
            while (position < tokens.size()) {
                const std::string& token = tokens[position++];
                if (token == ";") {
                    complete = true;
                    if (directive == "include") {
                        if (arguments.size() != 1U) {
                            throw std::runtime_error(
                                "Nginx include requires one path");
                        }
                        if (include_depth >= max_include_depth) {
                            throw std::runtime_error(
                                "Nginx include nesting exceeds the limit");
                        }
                        for (const auto& included :
                             resolve_include(arguments.front())) {
                            parse_file(included, contexts, include_depth + 1U);
                        }
                    } else if (directive == "server_name" &&
                               !contexts.empty() &&
                               contexts.back() == "server") {
                        add_server_names(arguments);
                    }
                    break;
                }
                if (token == "{") {
                    if (block_depth >= max_block_depth) {
                        throw std::runtime_error(
                            "Nginx config block nesting exceeds the limit");
                    }
                    contexts.push_back(directive);
                    parse_sequence(tokens, position, contexts, true,
                                   block_depth + 1U, include_depth);
                    contexts.pop_back();
                    complete = true;
                    break;
                }
                if (token == "}") {
                    throw std::runtime_error(
                        "Nginx config directive is missing a semicolon");
                }
                arguments.push_back(token);
            }
            if (!complete) {
                throw std::runtime_error(
                    "Nginx config directive is missing a terminator");
            }
        }
        if (expects_close) {
            throw std::runtime_error("Nginx config has an unclosed block");
        }
    }

    void parse_file(const std::filesystem::path& path,
                    std::vector<std::string> contexts,
                    std::size_t include_depth) {
        if (include_depth > max_include_depth) {
            throw std::runtime_error("Nginx include nesting exceeds the limit");
        }
        std::error_code error;
        const std::filesystem::path canonical =
            std::filesystem::weakly_canonical(path, error);
        if (error) {
            throw std::runtime_error("cannot resolve Nginx config path: " +
                                     path.string());
        }
        if (active_files_.contains(canonical)) {
            return;
        }
        if (file_parse_count_ >= max_file_parses) {
            throw std::runtime_error("Nginx config includes too many files");
        }
        ++file_parse_count_;
        const bool previously_parsed = parsed_files_.contains(canonical);
        if (!previously_parsed && parsed_files_.size() >= max_files) {
            throw std::runtime_error("Nginx config includes too many files");
        }
        const std::uintmax_t file_size =
            std::filesystem::file_size(canonical, error);
        if (error || file_size > max_file_size) {
            throw std::runtime_error("Nginx config file is missing or too large: " +
                                     canonical.string());
        }
        if (file_size > max_total_size - total_size_) {
            throw std::runtime_error("Nginx config tree exceeds the size limit");
        }
        total_size_ += static_cast<std::size_t>(file_size);
        parsed_files_.insert(canonical);

        active_files_.insert(canonical);
        const std::string contents =
            read_file(canonical, static_cast<std::size_t>(file_size));
        const std::vector<std::string> tokens = tokenize(contents);
        std::size_t position = 0;
        parse_sequence(tokens, position, contexts, false, 0U, include_depth);
        active_files_.erase(canonical);
    }
};

} // namespace

std::vector<std::string> discover_nginx_server_names(
    const std::filesystem::path& main_config) {
    NginxConfigScanner scanner{main_config};
    return scanner.scan(main_config);
}

} // namespace subsite_monitor
