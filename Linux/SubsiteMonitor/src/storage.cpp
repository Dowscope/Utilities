#include "storage.hpp"

#include "log_parser.hpp"
#include "report.hpp"

#include <algorithm>
#include <array>
#include <charconv>
#include <cerrno>
#include <cstdlib>
#include <fcntl.h>
#include <fstream>
#include <limits>
#include <map>
#include <set>
#include <stdexcept>
#include <string_view>
#include <system_error>
#include <utility>
#include <vector>

#include <sys/file.h>
#include <sys/stat.h>
#include <unistd.h>

namespace subsite_monitor {
namespace {

constexpr std::size_t max_csv_file_size = 8U * 1024U * 1024U;
constexpr std::size_t max_csv_line_size = 1024U;
constexpr std::size_t max_request_csv_line_size = 20000U;
constexpr std::size_t max_checkpoint_size = 8U * 1024U * 1024U;
constexpr std::size_t max_csv_files = 10000U;
constexpr std::size_t max_sites = 10000U;
constexpr std::size_t max_site_day_buckets = 100000U;
constexpr std::size_t max_open_request_files = 64U;

struct CsvSiteRow {
    std::uint64_t requests{};
    std::chrono::sys_seconds latest_access_utc{};
};

std::runtime_error path_error(std::string_view action,
                              const std::filesystem::path& path,
                              int error_number = errno) {
    return std::runtime_error(std::string{action} + " '" + path.string() +
                              "': " +
                              std::system_category().message(error_number));
}

bool valid_date(std::string_view value) {
    if (value.size() != 10 || value[4] != '-' || value[7] != '-') {
        return false;
    }
    for (std::size_t i = 0; i < value.size(); ++i) {
        if (i != 4U && i != 7U && (value[i] < '0' || value[i] > '9')) {
            return false;
        }
    }
    return parse_utc_timestamp(std::string{value} + "T00:00:00Z").has_value();
}

bool valid_hostname(std::string_view value) {
    if (value.empty() || value.size() > 253U || value.front() == '.' ||
        value.back() == '.') {
        return false;
    }
    for (const char character : value) {
        const bool alpha = (character >= 'a' && character <= 'z') ||
                           (character >= 'A' && character <= 'Z');
        const bool digit = character >= '0' && character <= '9';
        if (!alpha && !digit && character != '.' && character != '-' &&
            character != '_') {
            return false;
        }
    }
    return true;
}

class StoreLock {
public:
    StoreLock(const std::filesystem::path& directory, int lock_mode) {
        const std::filesystem::path lock_path = directory / ".lock";
        descriptor_ = open(lock_path.c_str(), O_CREAT | O_RDWR | O_CLOEXEC |
                                                    O_NOFOLLOW,
                            S_IRUSR | S_IWUSR);
        if (descriptor_ < 0) {
            throw path_error("cannot open statistics lock", lock_path);
        }
        struct stat status {};
        if (fstat(descriptor_, &status) != 0 || !S_ISREG(status.st_mode) ||
            status.st_uid != geteuid() || (status.st_mode & 0077) != 0) {
            close(descriptor_);
            descriptor_ = -1;
            throw std::runtime_error(
                "statistics lock must be a private file owned by the current user");
        }
        if (flock(descriptor_, lock_mode) != 0) {
            const int error_number = errno;
            close(descriptor_);
            descriptor_ = -1;
            throw path_error("cannot lock statistics directory", directory,
                             error_number);
        }
    }

    StoreLock(const StoreLock&) = delete;
    StoreLock& operator=(const StoreLock&) = delete;

    ~StoreLock() {
        if (descriptor_ >= 0) {
            (void)flock(descriptor_, LOCK_UN);
            (void)close(descriptor_);
        }
    }

private:
    int descriptor_{-1};
};

void prepare_directory(const std::filesystem::path& directory) {
    if (directory.empty()) {
        throw std::runtime_error("statistics directory must not be empty");
    }
    struct stat before_status {};
    const bool existed = lstat(directory.c_str(), &before_status) == 0;
    if (!existed && errno != ENOENT) {
        throw path_error("cannot inspect statistics directory", directory);
    }
    std::error_code error;
    std::filesystem::create_directories(directory, error);
    if (error) {
        throw std::runtime_error("cannot create statistics directory '" +
                                 directory.string() + "': " + error.message());
    }

    struct stat status {};
    if (lstat(directory.c_str(), &status) != 0) {
        throw path_error("cannot inspect statistics directory", directory);
    }
    if (!S_ISDIR(status.st_mode) || status.st_uid != geteuid()) {
        throw std::runtime_error(
            "statistics directory must be a real directory owned by the current user");
    }
    if (existed && (status.st_mode & 0077) != 0) {
        throw std::runtime_error(
            "existing statistics directory must already be private");
    }
    if (!existed && chmod(directory.c_str(), S_IRWXU) != 0) {
        throw path_error("cannot restrict statistics directory permissions",
                         directory);
    }
}

bool private_existing_directory(const std::filesystem::path& directory) {
    struct stat status {};
    if (lstat(directory.c_str(), &status) != 0) {
        if (errno == ENOENT) {
            return false;
        }
        throw path_error("cannot inspect statistics directory", directory);
    }
    if (!S_ISDIR(status.st_mode) || status.st_uid != geteuid() ||
        (status.st_mode & 0077) != 0) {
        throw std::runtime_error(
            "statistics directory must be private and owned by the current user");
    }
    return true;
}

std::string csv_escape(std::string_view value) {
    const bool quote = value.find_first_of(",\"\r\n") !=
                       std::string_view::npos;
    if (!quote) {
        return std::string{value};
    }
    std::string escaped;
    escaped.reserve(value.size() + 2U);
    escaped.push_back('"');
    for (const char character : value) {
        if (character == '"') {
            escaped.push_back('"');
        }
        escaped.push_back(character);
    }
    escaped.push_back('"');
    return escaped;
}

template <std::size_t FieldCount>
bool parse_csv_row(std::string_view line,
                   std::array<std::string, FieldCount>& fields) {
    std::size_t field = 0;
    std::size_t position = 0;
    while (true) {
        if (field >= fields.size()) {
            return false;
        }
        std::string& value = fields[field];
        if (position < line.size() && line[position] == '"') {
            ++position;
            bool closed = false;
            while (position < line.size()) {
                const char character = line[position++];
                if (character != '"') {
                    value.push_back(character);
                } else if (position < line.size() && line[position] == '"') {
                    value.push_back('"');
                    ++position;
                } else {
                    closed = true;
                    break;
                }
            }
            if (!closed || (position < line.size() && line[position] != ',')) {
                return false;
            }
        } else {
            const std::size_t delimiter = line.find(',', position);
            const std::size_t end = delimiter == std::string_view::npos
                                        ? line.size()
                                        : delimiter;
            const std::string_view plain = line.substr(position, end - position);
            if (plain.find('"') != std::string_view::npos) {
                return false;
            }
            value.assign(plain);
            position = end;
        }
        ++field;
        if (position == line.size()) {
            return field == fields.size();
        }
        if (line[position] != ',') {
            return false;
        }
        ++position;
    }
}

bool read_bounded_line(std::istream& input, std::string& line,
                       std::size_t max_line_size = max_csv_line_size) {
    line.clear();
    bool got_character = false;
    char character = '\0';
    while (input.get(character)) {
        if (character == '\n') {
            return true;
        }
        got_character = true;
        if (line.size() >= max_line_size) {
            throw std::runtime_error("statistics CSV contains an overlong line");
        }
        line.push_back(character);
    }
    if (input.bad()) {
        throw std::runtime_error("error while reading statistics CSV");
    }
    return got_character;
}

std::vector<std::string> list_csv_dates(
    const std::filesystem::path& directory) {
    std::vector<std::string> dates;
    std::error_code error;
    std::size_t entry_count = 0;
    for (std::filesystem::directory_iterator iterator{directory, error}, end;
         !error && iterator != end; iterator.increment(error)) {
        if (++entry_count > max_csv_files * 4U) {
            throw std::runtime_error("statistics directory has too many entries");
        }
        const std::string filename = iterator->path().filename().string();
        if (filename.size() != 14U || filename.substr(10) != ".csv") {
            continue;
        }
        const std::string date = filename.substr(0, 10);
        if (!valid_date(date)) {
            continue;
        }
        std::error_code type_error;
        if (!iterator->is_regular_file(type_error) || type_error) {
            continue;
        }
        dates.push_back(date);
        if (dates.size() > max_csv_files) {
            throw std::runtime_error("statistics store has too many daily CSV files");
        }
    }
    if (error) {
        throw std::runtime_error("cannot scan statistics directory '" +
                                 directory.string() + "': " + error.message());
    }
    std::sort(dates.begin(), dates.end(), std::greater<>{});
    return dates;
}

std::map<std::string, CsvSiteRow> read_day_file(
    const std::filesystem::path& path) {
    struct stat status {};
    if (lstat(path.c_str(), &status) != 0) {
        throw path_error("cannot inspect statistics CSV", path);
    }
    if (!S_ISREG(status.st_mode) || status.st_uid != geteuid() ||
        (status.st_mode & 0077) != 0 || status.st_size < 0 ||
        static_cast<unsigned long long>(status.st_size) > max_csv_file_size) {
        throw std::runtime_error("statistics CSV is not a private regular file: '" +
                                 path.string() + "'");
    }
    std::ifstream input{path, std::ios::binary};
    if (!input) {
        throw path_error("cannot open statistics CSV", path);
    }
    std::string line;
    if (!read_bounded_line(input, line) ||
        line != "site,requests,last_access") {
        throw std::runtime_error("invalid statistics CSV header: '" +
                                 path.string() + "'");
    }

    std::map<std::string, CsvSiteRow> rows;
    while (read_bounded_line(input, line)) {
        if (line.empty()) {
            continue;
        }
        std::array<std::string, 3> fields;
        if (!parse_csv_row(line, fields) || !valid_hostname(fields[0])) {
            throw std::runtime_error("invalid statistics CSV row: '" +
                                     path.string() + "'");
        }
        std::uint64_t requests = 0;
        const char* first = fields[1].data();
        const char* last = first + fields[1].size();
        const auto [parsed_end, parse_error] =
            std::from_chars(first, last, requests);
        if (parse_error != std::errc{} || parsed_end != last || requests == 0) {
            throw std::runtime_error("invalid request count in statistics CSV: '" +
                                     path.string() + "'");
        }
        const auto latest = parse_utc_timestamp(fields[2]);
        if (!latest) {
            throw std::runtime_error("invalid last_access in statistics CSV: '" +
                                     path.string() + "'");
        }
        if (!rows.emplace(std::move(fields[0]),
                          CsvSiteRow{requests, *latest}).second) {
            throw std::runtime_error("duplicate site row in statistics CSV: '" +
                                     path.string() + "'");
        }
    }
    return rows;
}

bool valid_client_ip(std::string_view value) {
    if (value.empty() || value.size() > 64U) {
        return false;
    }
    for (const char character : value) {
        const bool alpha = (character >= 'a' && character <= 'z') ||
                           (character >= 'A' && character <= 'Z');
        const bool digit = character >= '0' && character <= '9';
        if (!alpha && !digit && character != '.' && character != ':' &&
            character != '%' && character != '-' && character != '_') {
            return false;
        }
    }
    return true;
}

bool valid_method(std::string_view value) {
    if (value.empty() || value.size() > 32U) {
        return false;
    }
    constexpr std::string_view punctuation = "!#$%&'*+-.^_`|~";
    for (const char character : value) {
        const bool alpha = (character >= 'a' && character <= 'z') ||
                           (character >= 'A' && character <= 'Z');
        const bool digit = character >= '0' && character <= '9';
        if (!alpha && !digit &&
            punctuation.find(character) == std::string_view::npos) {
            return false;
        }
    }
    return true;
}

std::vector<std::string> list_request_dates(
    const std::filesystem::path& directory) {
    std::vector<std::string> dates;
    std::error_code error;
    std::size_t entry_count = 0;
    for (std::filesystem::directory_iterator iterator{directory, error}, end;
         !error && iterator != end; iterator.increment(error)) {
        if (++entry_count > max_csv_files * 4U) {
            throw std::runtime_error("statistics directory has too many entries");
        }
        const std::string filename = iterator->path().filename().string();
        constexpr std::string_view prefix = "requests-";
        if (filename.size() != prefix.size() + 14U ||
            !filename.starts_with(prefix) ||
            filename.substr(prefix.size() + 10U) != ".csv") {
            continue;
        }
        const std::string date = filename.substr(prefix.size(), 10U);
        if (!valid_date(date)) {
            continue;
        }
        std::error_code type_error;
        if (!iterator->is_regular_file(type_error) || type_error) {
            continue;
        }
        dates.push_back(date);
        if (dates.size() > max_csv_files) {
            throw std::runtime_error("statistics store has too many request CSV files");
        }
    }
    if (error) {
        throw std::runtime_error("cannot scan statistics directory '" +
                                 directory.string() + "': " + error.message());
    }
    std::sort(dates.begin(), dates.end());
    return dates;
}

std::chrono::sys_seconds validate_request_fields(
    const std::array<std::string, 6>& fields,
    const std::filesystem::path& path) {
    const auto timestamp = parse_utc_timestamp(fields[0]);
    if (!timestamp || !valid_hostname(fields[1]) ||
        !valid_client_ip(fields[2]) || !valid_method(fields[3]) ||
        fields[4].empty() || fields[4].size() > 8192U) {
        throw std::runtime_error("invalid request record in CSV: '" +
                                 path.string() + "'");
    }
    for (const unsigned char character : fields[4]) {
        if (character <= 0x20U || character == 0x7fU) {
            throw std::runtime_error("invalid URL in request CSV: '" +
                                     path.string() + "'");
        }
    }
    unsigned status = 0;
    const auto [end, error] = std::from_chars(
        fields[5].data(), fields[5].data() + fields[5].size(), status);
    if (error != std::errc{} || end != fields[5].data() + fields[5].size() ||
        status < 100U || status > 599U) {
        throw std::runtime_error("invalid status in request CSV: '" +
                                 path.string() + "'");
    }
    return *timestamp;
}

template <typename Handler>
void scan_request_file(const std::filesystem::path& path,
                       std::string_view date,
                       std::optional<std::string_view> requested_hostname,
                       Handler&& handler) {
    struct stat status {};
    if (lstat(path.c_str(), &status) != 0) {
        throw path_error("cannot inspect request CSV", path);
    }
    if (!S_ISREG(status.st_mode) || status.st_uid != geteuid() ||
        (status.st_mode & 0077) != 0) {
        throw std::runtime_error("request CSV is not a private regular file: '" +
                                 path.string() + "'");
    }
    std::ifstream input{path, std::ios::binary};
    if (!input) {
        throw path_error("cannot open request CSV", path);
    }
    std::string line;
    if (!read_bounded_line(input, line, max_request_csv_line_size) ||
        line != "timestamp_utc,site,client_ip,method,url,status") {
        throw std::runtime_error("invalid request CSV header: '" +
                                 path.string() + "'");
    }
    while (read_bounded_line(input, line, max_request_csv_line_size)) {
        if (line.empty()) {
            continue;
        }
        std::array<std::string, 6> fields;
        if (!parse_csv_row(line, fields)) {
            throw std::runtime_error("invalid request CSV row: '" +
                                     path.string() + "'");
        }
        const std::chrono::sys_seconds timestamp =
            validate_request_fields(fields, path);
        if (!requested_hostname || fields[1] == *requested_hostname) {
            unsigned status_code = 0;
            (void)std::from_chars(fields[5].data(),
                                  fields[5].data() + fields[5].size(),
                                  status_code);
            if (!handler(RequestRecord{
                    std::string{date}, std::move(fields[1]), timestamp,
                    std::move(fields[2]),
                    std::move(fields[3]), std::move(fields[4]),
                    static_cast<std::uint16_t>(status_code)})) {
                break;
            }
        }
    }
}

std::vector<std::string> selected_dates(
    const std::filesystem::path& directory, std::size_t days) {
    std::vector<std::string> dates = list_csv_dates(directory);
    if (days > 0 && dates.size() > days) {
        dates.resize(days);
    }
    return dates;
}

void append_csv_value(std::string& output, std::string_view value) {
    output += csv_escape(value);
}

void write_all(int descriptor, std::string_view data,
               const std::filesystem::path& path) {
    while (!data.empty()) {
        const ssize_t written = write(descriptor, data.data(), data.size());
        if (written < 0) {
            if (errno == EINTR) {
                continue;
            }
            throw path_error("cannot write temporary statistics CSV", path);
        }
        if (written == 0) {
            throw std::runtime_error("short write while storing statistics CSV");
        }
        data.remove_prefix(static_cast<std::size_t>(written));
    }
}

void replace_daily_file(const std::filesystem::path& directory,
                        const std::string& date,
                        const std::map<std::string, CsvSiteRow>& rows) {
    std::string contents = "site,requests,last_access\n";
    for (const auto& [hostname, row] : rows) {
        append_csv_value(contents, hostname);
        contents.push_back(',');
        contents += std::to_string(row.requests);
        contents.push_back(',');
        contents += format_timestamp_utc(row.latest_access_utc);
        contents.push_back('\n');
        if (contents.size() > max_csv_file_size) {
            throw std::runtime_error("daily statistics CSV exceeds the size limit");
        }
    }

    std::string temporary_template =
        (directory / ("." + date + ".csv.tmp.XXXXXX")).string();
    std::vector<char> temporary_name(temporary_template.begin(),
                                     temporary_template.end());
    temporary_name.push_back('\0');
    const int descriptor = mkstemp(temporary_name.data());
    if (descriptor < 0) {
        throw path_error("cannot create temporary statistics CSV", directory);
    }
    const std::filesystem::path temporary_path{temporary_name.data()};
    bool keep_temporary = true;
    bool descriptor_open = true;
    try {
        if (fchmod(descriptor, S_IRUSR | S_IWUSR) != 0) {
            throw path_error("cannot restrict temporary CSV permissions",
                             temporary_path);
        }
        write_all(descriptor, contents, temporary_path);
        if (fsync(descriptor) != 0) {
            throw path_error("cannot sync temporary statistics CSV",
                             temporary_path);
        }
        if (close(descriptor) != 0) {
            descriptor_open = false;
            throw path_error("cannot close temporary statistics CSV",
                             temporary_path);
        }
        descriptor_open = false;
        const std::filesystem::path destination = directory / (date + ".csv");
        std::error_code error;
        std::filesystem::rename(temporary_path, destination, error);
        if (error) {
            throw std::runtime_error("cannot replace statistics CSV '" +
                                     destination.string() + "': " + error.message());
        }
        keep_temporary = false;
        const int directory_fd = open(directory.c_str(), O_RDONLY | O_DIRECTORY |
                                                               O_CLOEXEC);
        if (directory_fd < 0) {
            throw path_error("cannot open statistics directory", directory);
        }
        const int sync_result = fsync(directory_fd);
        const int sync_error = errno;
        (void)close(directory_fd);
        if (sync_result != 0) {
            throw path_error("cannot sync statistics directory", directory,
                             sync_error);
        }
    } catch (...) {
        if (descriptor_open) {
            (void)close(descriptor);
        }
        if (keep_temporary) {
            (void)unlink(temporary_path.c_str());
        }
        throw;
    }
}

class DailyRequestWriter {
public:
    explicit DailyRequestWriter(std::filesystem::path directory)
        : directory_(std::move(directory)) {}

    DailyRequestWriter(const DailyRequestWriter&) = delete;
    DailyRequestWriter& operator=(const DailyRequestWriter&) = delete;

    ~DailyRequestWriter() {
        for (auto& [date, file] : files_) {
            (void)date;
            close_file(file);
            if (file.temporary_exists) {
                (void)unlink(file.temporary_path.c_str());
            }
        }
    }

    void write(const AccessLogEntry& entry) {
        const std::string date = entry.timestamp.substr(0, 10);
        auto position = files_.find(date);
        if (position == files_.end()) {
            position = files_.try_emplace(date).first;
            try {
                position->second = make_file(date);
            } catch (...) {
                files_.erase(position);
                throw;
            }
        }
        ensure_open(position->second);
        const std::string row = csv_escape(format_timestamp_utc(entry.timestamp_utc)) +
                                "," + csv_escape(entry.hostname) + "," +
                                csv_escape(entry.client_ip) + "," +
                                csv_escape(entry.method) + "," +
                                csv_escape(entry.target) + "," +
                                std::to_string(entry.status) + "\n";
        write_all(position->second.descriptor, row,
                  position->second.temporary_path);
        position->second.last_used = ++use_counter_;
    }

    void finish() {
        for (auto& [date, file] : files_) {
            (void)date;
            ensure_open(file);
            if (fsync(file.descriptor) != 0) {
                throw path_error("cannot sync request CSV", file.temporary_path);
            }
            close_file(file);
        }

        for (auto& [date, file] : files_) {
            const std::filesystem::path destination =
                directory_ / ("requests-" + date + ".csv");
            std::error_code error;
            std::filesystem::rename(file.temporary_path, destination, error);
            if (error) {
                throw std::runtime_error("cannot replace request CSV '" +
                                         destination.string() + "': " +
                                         error.message());
            }
            file.temporary_exists = false;
        }

        const int directory_fd = open(directory_.c_str(), O_RDONLY | O_DIRECTORY |
                                                             O_CLOEXEC);
        if (directory_fd < 0) {
            throw path_error("cannot open statistics directory", directory_);
        }
        const int sync_result = fsync(directory_fd);
        const int sync_error = errno;
        (void)close(directory_fd);
        if (sync_result != 0) {
            throw path_error("cannot sync statistics directory", directory_,
                             sync_error);
        }
    }

private:
    struct TempFile {
        std::filesystem::path temporary_path;
        int descriptor{-1};
        std::uint64_t last_used{};
        bool temporary_exists{true};
    };

    TempFile make_file(const std::string& date) {
        std::string pattern =
            (directory_ / (".requests-" + date + ".csv.tmp.XXXXXX")).string();
        std::vector<char> writable(pattern.begin(), pattern.end());
        writable.push_back('\0');
        const int descriptor = mkstemp(writable.data());
        if (descriptor < 0) {
            throw path_error("cannot create temporary request CSV", directory_);
        }
        TempFile file{writable.data(), descriptor, ++use_counter_, true};
        ++open_file_count_;
        if (fchmod(descriptor, S_IRUSR | S_IWUSR) != 0) {
            const int error_number = errno;
            close_file(file);
            (void)unlink(file.temporary_path.c_str());
            file.temporary_exists = false;
            throw path_error("cannot restrict request CSV permissions",
                             file.temporary_path, error_number);
        }
        constexpr std::string_view header =
            "timestamp_utc,site,client_ip,method,url,status\n";
        try {
            write_all(descriptor, header, file.temporary_path);
        } catch (...) {
            close_file(file);
            (void)unlink(file.temporary_path.c_str());
            file.temporary_exists = false;
            throw;
        }
        evict_if_needed(&file);
        return file;
    }

    void close_file(TempFile& file) noexcept {
        if (file.descriptor >= 0) {
            (void)close(file.descriptor);
            file.descriptor = -1;
            if (open_file_count_ > 0U) {
                --open_file_count_;
            }
        }
    }

    void evict_if_needed(
        const TempFile* keep_open,
        std::size_t target_count = max_open_request_files) {
        while (open_file_count_ > target_count) {
            auto oldest = files_.end();
            for (auto iterator = files_.begin(); iterator != files_.end();
                 ++iterator) {
                if (&iterator->second == keep_open ||
                    iterator->second.descriptor < 0) {
                    continue;
                }
                if (oldest == files_.end() ||
                    iterator->second.last_used < oldest->second.last_used) {
                    oldest = iterator;
                }
            }
            if (oldest == files_.end()) {
                break;
            }
            close_file(oldest->second);
        }
    }

    void ensure_open(TempFile& file) {
        if (file.descriptor >= 0) {
            file.last_used = ++use_counter_;
            return;
        }
        if (open_file_count_ >= max_open_request_files) {
            evict_if_needed(nullptr, max_open_request_files - 1U);
        }
        file.descriptor = open(file.temporary_path.c_str(),
                               O_WRONLY | O_APPEND | O_CLOEXEC | O_NOFOLLOW);
        if (file.descriptor < 0) {
            throw path_error("cannot reopen temporary request CSV",
                             file.temporary_path);
        }
        ++open_file_count_;
        file.last_used = ++use_counter_;
    }

    std::filesystem::path directory_;
    std::map<std::string, TempFile> files_;
    std::size_t open_file_count_{};
    std::uint64_t use_counter_{};
};

std::filesystem::path checkpoint_path(
    const std::filesystem::path& directory) {
    return directory / ".monitor-checkpoint";
}

void sync_directory(const std::filesystem::path& directory) {
    const int directory_fd = open(directory.c_str(), O_RDONLY | O_DIRECTORY |
                                                         O_CLOEXEC);
    if (directory_fd < 0) {
        throw path_error("cannot open statistics directory", directory);
    }
    const int sync_result = fsync(directory_fd);
    const int sync_error = errno;
    (void)close(directory_fd);
    if (sync_result != 0) {
        throw path_error("cannot sync statistics directory", directory,
                         sync_error);
    }
}

bool existing_regular_private_file(const std::filesystem::path& path,
                                   struct stat& status) {
    if (lstat(path.c_str(), &status) != 0) {
        if (errno == ENOENT) {
            return false;
        }
        throw path_error("cannot inspect monitor checkpoint", path);
    }
    if (!S_ISREG(status.st_mode) || status.st_uid != geteuid() ||
        (status.st_mode & 0077) != 0 || status.st_size < 0 ||
        static_cast<unsigned long long>(status.st_size) > max_checkpoint_size) {
        throw std::runtime_error("monitor checkpoint is not a private regular file");
    }
    return true;
}

std::optional<MonitorCheckpoint> load_checkpoint_unlocked(
    const std::filesystem::path& directory) {
    const std::filesystem::path path = checkpoint_path(directory);
    struct stat status {};
    if (!existing_regular_private_file(path, status)) {
        return std::nullopt;
    }
    std::ifstream input{path};
    if (!input) {
        throw path_error("cannot read monitor checkpoint", path);
    }
    std::string line;
    if (!std::getline(input, line) || line != "SUBSITE_MONITOR_CHECKPOINT 1") {
        throw std::runtime_error("invalid monitor checkpoint header");
    }
    if (!std::getline(input, line)) {
        throw std::runtime_error("monitor checkpoint is missing its cursor");
    }
    std::istringstream cursor_line{line};
    std::string key;
    int discard = 0;
    MonitorCheckpoint checkpoint;
    std::string trailing;
    if (!(cursor_line >> key >> checkpoint.device >> checkpoint.inode >>
          checkpoint.offset >> discard) || key != "cursor" ||
        (discard != 0 && discard != 1) || (cursor_line >> trailing)) {
        throw std::runtime_error("invalid monitor checkpoint cursor");
    }
    checkpoint.discard_until_newline = discard == 1;

    std::size_t file_count = 0;
    while (std::getline(input, line)) {
        if (line.size() > max_csv_line_size) {
            throw std::runtime_error("monitor checkpoint line is too long");
        }
        if (line.empty()) {
            continue;
        }
        std::istringstream file_line{line};
        std::string date;
        std::uint64_t size = 0;
        trailing.clear();
        if (!(file_line >> key >> date >> size) || key != "file" ||
            !valid_date(date) || (file_line >> trailing) ||
            !checkpoint.request_file_sizes.emplace(date, size).second) {
            throw std::runtime_error("invalid monitor checkpoint file entry");
        }
        if (++file_count > max_csv_files) {
            throw std::runtime_error("monitor checkpoint lists too many files");
        }
    }
    if (input.bad()) {
        throw std::runtime_error("error while reading monitor checkpoint");
    }
    return checkpoint;
}

void save_checkpoint_unlocked(const std::filesystem::path& directory,
                             const MonitorCheckpoint& checkpoint) {
    std::string contents = "SUBSITE_MONITOR_CHECKPOINT 1\ncursor " +
                          std::to_string(checkpoint.device) + " " +
                          std::to_string(checkpoint.inode) + " " +
                          std::to_string(checkpoint.offset) + " " +
                          (checkpoint.discard_until_newline ? "1\n" : "0\n");
    for (const auto& [date, size] : checkpoint.request_file_sizes) {
        if (!valid_date(date)) {
            throw std::runtime_error("invalid request CSV date in checkpoint");
        }
        contents += "file " + date + " " + std::to_string(size) + "\n";
        if (contents.size() > max_checkpoint_size) {
            throw std::runtime_error("monitor checkpoint exceeds the size limit");
        }
    }

    std::string pattern = (directory / ".monitor-checkpoint.tmp.XXXXXX").string();
    std::vector<char> writable(pattern.begin(), pattern.end());
    writable.push_back('\0');
    const int descriptor = mkstemp(writable.data());
    if (descriptor < 0) {
        throw path_error("cannot create monitor checkpoint", directory);
    }
    const std::filesystem::path temporary_path{writable.data()};
    bool descriptor_open = true;
    bool keep_temporary = true;
    try {
        if (fchmod(descriptor, S_IRUSR | S_IWUSR) != 0) {
            throw path_error("cannot restrict monitor checkpoint permissions",
                             temporary_path);
        }
        write_all(descriptor, contents, temporary_path);
        if (fsync(descriptor) != 0) {
            throw path_error("cannot sync monitor checkpoint", temporary_path);
        }
        if (close(descriptor) != 0) {
            descriptor_open = false;
            throw path_error("cannot close monitor checkpoint", temporary_path);
        }
        descriptor_open = false;
        std::error_code error;
        std::filesystem::rename(temporary_path, checkpoint_path(directory), error);
        if (error) {
            throw std::runtime_error("cannot replace monitor checkpoint: " +
                                     error.message());
        }
        keep_temporary = false;
        sync_directory(directory);
    } catch (...) {
        if (descriptor_open) {
            (void)close(descriptor);
        }
        if (keep_temporary) {
            (void)unlink(temporary_path.c_str());
        }
        throw;
    }
}

void add_request_to_statistics(StatisticsReport& report,
                               const RequestRecord& record,
                               std::size_t& bucket_count) {
    auto site_position = report.sites.find(record.hostname);
    if (site_position == report.sites.end()) {
        if (report.sites.size() >= max_sites) {
            throw std::runtime_error(
                "statistics limit exceeded: too many distinct websites");
        }
        site_position = report.sites.emplace(record.hostname, SiteStatistics{}).first;
    }
    SiteStatistics& site = site_position->second;
    if (site.requests == std::numeric_limits<std::uint64_t>::max()) {
        throw std::runtime_error("statistics limit exceeded: request count overflow");
    }
    ++site.requests;
    if (site.requests == 1 || record.timestamp_utc > site.latest_access_utc) {
        site.latest_access_utc = record.timestamp_utc;
    }
    auto day_position = site.requests_by_day.find(record.date);
    if (day_position == site.requests_by_day.end()) {
        if (bucket_count >= max_site_day_buckets) {
            throw std::runtime_error(
                "statistics limit exceeded: too many website/day pairs");
        }
        day_position = site.requests_by_day.emplace(record.date, 0).first;
        ++bucket_count;
    }
    if (day_position->second == std::numeric_limits<std::uint64_t>::max()) {
        throw std::runtime_error("statistics limit exceeded: request count overflow");
    }
    ++day_position->second;
    auto latest_position = site.latest_access_by_day.find(record.date);
    if (latest_position == site.latest_access_by_day.end() ||
        record.timestamp_utc > latest_position->second) {
        site.latest_access_by_day[record.date] = record.timestamp_utc;
    }
}

StatisticsReport rebuild_statistics_unlocked(
    const std::filesystem::path& directory) {
    StatisticsReport report;
    std::size_t bucket_count = 0;
    for (const std::string& date : list_request_dates(directory)) {
        const std::filesystem::path path =
            directory / ("requests-" + date + ".csv");
        scan_request_file(path, date, std::nullopt,
                          [&](RequestRecord record) {
                              add_request_to_statistics(report, record,
                                                        bucket_count);
                              return true;
                          });
    }
    return report;
}

void write_summary_dates_unlocked(const StatisticsReport& report,
                                  const std::filesystem::path& directory,
                                  const std::set<std::string>& dates) {
    for (const std::string& date : dates) {
        std::map<std::string, CsvSiteRow> rows;
        for (const auto& [hostname, site] : report.sites) {
            const auto count = site.requests_by_day.find(date);
            if (count == site.requests_by_day.end()) {
                continue;
            }
            const auto latest = site.latest_access_by_day.find(date);
            if (latest == site.latest_access_by_day.end()) {
                throw std::runtime_error("statistics report is missing a daily timestamp");
            }
            rows.emplace(hostname, CsvSiteRow{count->second, latest->second});
        }
        if (!rows.empty()) {
            replace_daily_file(directory, date, rows);
        }
    }
}

void restore_request_files_unlocked(
    const std::filesystem::path& directory,
    const MonitorCheckpoint& checkpoint) {
    const std::vector<std::string> existing_dates = list_request_dates(directory);
    std::set<std::string> existing(existing_dates.begin(), existing_dates.end());
    for (const auto& [date, expected_size] : checkpoint.request_file_sizes) {
        const std::filesystem::path path =
            directory / ("requests-" + date + ".csv");
        struct stat status {};
        if (lstat(path.c_str(), &status) != 0) {
            throw path_error("cannot inspect checkpointed request CSV", path);
        }
        if (!S_ISREG(status.st_mode) || status.st_uid != geteuid() ||
            (status.st_mode & 0077) != 0 || status.st_size < 0 ||
            static_cast<std::uint64_t>(status.st_size) < expected_size) {
            throw std::runtime_error("checkpointed request CSV is missing data: '" +
                                     path.string() + "'");
        }
        if (static_cast<std::uint64_t>(status.st_size) > expected_size) {
            const int descriptor = open(path.c_str(), O_WRONLY | O_CLOEXEC |
                                                          O_NOFOLLOW);
            if (descriptor < 0) {
                throw path_error("cannot recover request CSV", path);
            }
            const int result = ftruncate(descriptor,
                                         static_cast<off_t>(expected_size));
            const int error_number = errno;
            if (result == 0 && fsync(descriptor) != 0) {
                (void)close(descriptor);
                throw path_error("cannot sync recovered request CSV", path);
            }
            (void)close(descriptor);
            if (result != 0) {
                throw path_error("cannot truncate uncommitted request data", path,
                                 error_number);
            }
        }
        existing.erase(date);
    }
    for (const std::string& date : existing) {
        const std::filesystem::path path =
            directory / ("requests-" + date + ".csv");
        if (unlink(path.c_str()) != 0) {
            throw path_error("cannot remove uncommitted request CSV", path);
        }
    }
    if (!existing.empty()) {
        sync_directory(directory);
    }
}

void append_request_entries_unlocked(
    const std::filesystem::path& directory,
    const std::vector<AccessLogEntry>& entries,
    MonitorCheckpoint& next_checkpoint) {
    std::map<std::string, std::vector<const AccessLogEntry*>> daily_entries;
    for (const AccessLogEntry& entry : entries) {
        daily_entries[entry.timestamp.substr(0, 10)].push_back(&entry);
    }
    for (const auto& [date, day_entries] : daily_entries) {
        const auto expected_position =
            next_checkpoint.request_file_sizes.find(date);
        const bool new_file = expected_position ==
                              next_checkpoint.request_file_sizes.end();
        const std::uint64_t expected_size =
            new_file ? 0U : expected_position->second;
        const std::filesystem::path path =
            directory / ("requests-" + date + ".csv");
        const int descriptor = open(path.c_str(), O_WRONLY | O_APPEND |
                                                      O_CLOEXEC | O_NOFOLLOW |
                                                      (new_file ? O_CREAT | O_EXCL
                                                                : 0),
                                    S_IRUSR | S_IWUSR);
        if (descriptor < 0) {
            throw path_error("cannot open request CSV for monitoring", path);
        }
        struct stat status {};
        if (fstat(descriptor, &status) != 0 || !S_ISREG(status.st_mode) ||
            status.st_uid != geteuid() || (status.st_mode & 0077) != 0 ||
            status.st_size < 0 ||
            static_cast<std::uint64_t>(status.st_size) != expected_size) {
            (void)close(descriptor);
            throw std::runtime_error("request CSV size does not match checkpoint: '" +
                                     path.string() + "'");
        }
        std::uint64_t new_size = expected_size;
        try {
            if (new_file) {
                constexpr std::string_view header =
                    "timestamp_utc,site,client_ip,method,url,status\n";
                write_all(descriptor, header, path);
                new_size += header.size();
            }
            for (const AccessLogEntry* entry : day_entries) {
                const std::string row =
                    csv_escape(format_timestamp_utc(entry->timestamp_utc)) +
                    "," + csv_escape(entry->hostname) + "," +
                    csv_escape(entry->client_ip) + "," +
                    csv_escape(entry->method) + "," +
                    csv_escape(entry->target) + "," +
                    std::to_string(entry->status) + "\n";
                write_all(descriptor, row, path);
                new_size += row.size();
            }
            if (fsync(descriptor) != 0) {
                throw path_error("cannot sync monitored request CSV", path);
            }
        } catch (...) {
            (void)close(descriptor);
            throw;
        }
        if (close(descriptor) != 0) {
            throw path_error("cannot close monitored request CSV", path);
        }
        next_checkpoint.request_file_sizes[date] = new_size;
    }
}

class OfflineCollectionLock {
public:
    explicit OfflineCollectionLock(const std::filesystem::path& directory) {
        const std::filesystem::path path = directory / ".service.lock";
        descriptor_ = open(path.c_str(), O_CREAT | O_RDWR | O_CLOEXEC |
                                               O_NOFOLLOW,
                           S_IRUSR | S_IWUSR);
        if (descriptor_ < 0) {
            throw path_error("cannot open monitor service lock", path);
        }
        struct stat status {};
        if (fstat(descriptor_, &status) != 0 || !S_ISREG(status.st_mode) ||
            status.st_uid != geteuid() || (status.st_mode & 0077) != 0) {
            (void)close(descriptor_);
            descriptor_ = -1;
            throw std::runtime_error("monitor service lock must be private");
        }
        if (flock(descriptor_, LOCK_EX | LOCK_NB) != 0) {
            const int error_number = errno;
            (void)close(descriptor_);
            descriptor_ = -1;
            if (error_number == EWOULDBLOCK || error_number == EAGAIN) {
                throw std::runtime_error(
                    "monitor service is running; stop it before collecting manually");
            }
            throw path_error("cannot lock monitor service", path, error_number);
        }
    }

    OfflineCollectionLock(const OfflineCollectionLock&) = delete;
    OfflineCollectionLock& operator=(const OfflineCollectionLock&) = delete;

    ~OfflineCollectionLock() {
        if (descriptor_ >= 0) {
            (void)flock(descriptor_, LOCK_UN);
            (void)close(descriptor_);
        }
    }

private:
    int descriptor_{-1};
};

} // namespace

std::filesystem::path default_statistics_directory() {
    if (geteuid() == 0) {
        return "/var/lib/subsite-monitor/stats";
    }
    const char* state_home = std::getenv("XDG_STATE_HOME");
    if (state_home != nullptr && state_home[0] == '/') {
        return std::filesystem::path{state_home} / "subsite-monitor" / "stats";
    }
    const char* home = std::getenv("HOME");
    if (home == nullptr || home[0] != '/') {
        throw std::runtime_error(
            "cannot determine a user statistics directory; set SUBSITE_MONITOR_STATS_DIR");
    }
    return std::filesystem::path{home} / ".local" / "state" /
           "subsite-monitor" / "stats";
}

StatisticsReport collect_and_store(const std::filesystem::path& log_path,
                                   const std::filesystem::path& directory) {
    prepare_directory(directory);
    OfflineCollectionLock offline_lock{directory};
    StoreLock lock{directory, LOCK_EX};
    DailyRequestWriter request_writer{directory};
    StatisticsReport report = build_statistics(
        log_path, [&request_writer](const AccessLogEntry& entry) {
            request_writer.write(entry);
        });

    std::map<std::string, std::map<std::string, CsvSiteRow>> daily_rows;
    for (const auto& [hostname, site] : report.sites) {
        for (const auto& [date, requests] : site.requests_by_day) {
            const auto latest = site.latest_access_by_day.find(date);
            if (!valid_date(date) || latest == site.latest_access_by_day.end() ||
                requests == 0) {
                throw std::runtime_error(
                    "statistics report contains an invalid daily aggregate");
            }
            daily_rows[date].emplace(
                hostname, CsvSiteRow{requests, latest->second});
        }
    }
    for (const auto& [date, rows] : daily_rows) {
        replace_daily_file(directory, date, rows);
    }
    request_writer.finish();
    const std::filesystem::path checkpoint = checkpoint_path(directory);
    if (unlink(checkpoint.c_str()) != 0 && errno != ENOENT) {
        throw path_error("cannot reset monitor checkpoint", checkpoint);
    }
    sync_directory(directory);
    return report;
}

StatisticsReport read_stored_statistics(const std::filesystem::path& directory,
                                         std::size_t days) {
    StatisticsReport report;
    if (!private_existing_directory(directory)) {
        return report;
    }
    StoreLock lock{directory, LOCK_SH};
    const std::vector<std::string> dates = selected_dates(directory, days);
    std::size_t bucket_count = 0;
    for (const std::string& date : dates) {
        const auto day_rows = read_day_file(directory / (date + ".csv"));
        for (const auto& [hostname, row] : day_rows) {
            auto site_position = report.sites.find(hostname);
            if (site_position == report.sites.end()) {
                if (report.sites.size() >= max_sites) {
                    throw std::runtime_error(
                        "statistics limit exceeded: too many distinct websites");
                }
                site_position = report.sites.emplace(hostname, SiteStatistics{}).first;
            }
            if (bucket_count >= max_site_day_buckets) {
                throw std::runtime_error(
                    "statistics limit exceeded: too many website/day pairs");
            }
            SiteStatistics& site = site_position->second;
            if (row.requests >
                std::numeric_limits<std::uint64_t>::max() - site.requests) {
                throw std::runtime_error(
                    "statistics limit exceeded: request count overflow");
            }
            const bool first_site_row = site.requests == 0;
            if (!first_site_row &&
                site.latest_access_by_day.contains(date)) {
                throw std::runtime_error("duplicate daily statistics row");
            }
            site.requests += row.requests;
            site.requests_by_day.emplace(date, row.requests);
            site.latest_access_by_day.emplace(date, row.latest_access_utc);
            if (first_site_row ||
                row.latest_access_utc > site.latest_access_utc) {
                site.latest_access_utc = row.latest_access_utc;
            }
            ++bucket_count;
        }
    }
    return report;
}

std::vector<SiteDayHistory> read_site_history(
    const std::filesystem::path& directory, const std::string& hostname,
    std::size_t days) {
    std::vector<SiteDayHistory> history;
    if (!private_existing_directory(directory)) {
        return history;
    }
    StoreLock lock{directory, LOCK_SH};
    for (const std::string& date : selected_dates(directory, days)) {
        const auto day_rows = read_day_file(directory / (date + ".csv"));
        const auto row = day_rows.find(hostname);
        if (row != day_rows.end()) {
            history.push_back(
                {date, row->second.requests, row->second.latest_access_utc});
        }
    }
    return history;
}

RequestPage read_site_requests(const std::filesystem::path& directory,
                               const std::string& hostname,
                               std::size_t offset, std::size_t limit) {
    if (limit == 0U || limit > 1000U ||
        offset > std::numeric_limits<std::size_t>::max() - limit) {
        throw std::runtime_error("request history page is outside supported limits");
    }
    RequestPage page;
    page.offset = offset;
    if (!private_existing_directory(directory)) {
        return page;
    }
    StoreLock lock{directory, LOCK_SH};
    std::size_t matched = 0;
    for (const std::string& date : list_request_dates(directory)) {
        const std::filesystem::path path =
            directory / ("requests-" + date + ".csv");
        scan_request_file(path, date, std::string_view{hostname},
                          [&](RequestRecord record) {
            if (matched >= offset && page.records.size() < limit) {
                page.records.push_back(std::move(record));
            } else if (page.records.size() == limit) {
                page.has_more = true;
                return false;
            }
            ++matched;
            return true;
                          });
        if (page.has_more) {
            break;
        }
    }
    return page;
}

bool monitor_service_is_active(const std::filesystem::path& directory) {
    const std::filesystem::path lock_path = directory / ".service.lock";
    const int descriptor = open(lock_path.c_str(), O_RDWR | O_CLOEXEC |
                                                       O_NOFOLLOW);
    if (descriptor < 0) {
        if (errno == ENOENT) {
            return false;
        }
        throw path_error("cannot inspect monitor service lock", lock_path);
    }
    struct stat status {};
    if (fstat(descriptor, &status) != 0 || !S_ISREG(status.st_mode) ||
        status.st_uid != geteuid() || (status.st_mode & 0077) != 0) {
        (void)close(descriptor);
        throw std::runtime_error("monitor service lock must be a private file");
    }
    if (flock(descriptor, LOCK_EX | LOCK_NB) == 0) {
        (void)flock(descriptor, LOCK_UN);
        (void)close(descriptor);
        return false;
    }
    const int error_number = errno;
    (void)close(descriptor);
    if (error_number == EWOULDBLOCK || error_number == EAGAIN) {
        return true;
    }
    throw path_error("cannot inspect monitor service lock", lock_path,
                     error_number);
}

std::optional<MonitorCheckpoint> load_monitor_checkpoint(
    const std::filesystem::path& directory) {
    if (!private_existing_directory(directory)) {
        return std::nullopt;
    }
    StoreLock lock{directory, LOCK_SH};
    return load_checkpoint_unlocked(directory);
}

MonitorCheckpoint initialize_monitor_checkpoint(
    const std::filesystem::path& directory, std::uint64_t device,
    std::uint64_t inode, std::uint64_t offset, bool discard_until_newline) {
    prepare_directory(directory);
    StoreLock lock{directory, LOCK_EX};
    if (load_checkpoint_unlocked(directory)) {
        throw std::runtime_error("monitor checkpoint already exists");
    }
    MonitorCheckpoint checkpoint{device, inode, offset, discard_until_newline, {}};
    for (const std::string& date : list_request_dates(directory)) {
        const std::filesystem::path path =
            directory / ("requests-" + date + ".csv");
        struct stat status {};
        if (lstat(path.c_str(), &status) != 0) {
            throw path_error("cannot inspect request CSV", path);
        }
        if (!S_ISREG(status.st_mode) || status.st_uid != geteuid() ||
            (status.st_mode & 0077) != 0 || status.st_size < 0) {
            throw std::runtime_error("request CSV must be a private regular file");
        }
        checkpoint.request_file_sizes.emplace(
            date, static_cast<std::uint64_t>(status.st_size));
    }
    save_checkpoint_unlocked(directory, checkpoint);
    return checkpoint;
}

StatisticsReport recover_monitor_store(
    const std::filesystem::path& directory,
    const MonitorCheckpoint& checkpoint) {
    StoreLock lock{directory, LOCK_EX};
    const auto current = load_checkpoint_unlocked(directory);
    if (!current || current->device != checkpoint.device ||
        current->inode != checkpoint.inode ||
        current->offset != checkpoint.offset ||
        current->discard_until_newline != checkpoint.discard_until_newline ||
        current->request_file_sizes != checkpoint.request_file_sizes) {
        throw std::runtime_error("monitor checkpoint changed during recovery");
    }
    restore_request_files_unlocked(directory, checkpoint);
    StatisticsReport report = rebuild_statistics_unlocked(directory);
    std::set<std::string> dates;
    for (const auto& [date, size] : checkpoint.request_file_sizes) {
        (void)size;
        dates.insert(date);
    }
    write_summary_dates_unlocked(report, directory, dates);
    return report;
}

MonitorCheckpoint commit_monitor_batch(
    const std::filesystem::path& directory,
    const MonitorCheckpoint& checkpoint, std::uint64_t device,
    std::uint64_t inode, std::uint64_t offset, bool discard_until_newline,
    const std::vector<AccessLogEntry>& entries,
    const StatisticsReport& updated_statistics) {
    StoreLock lock{directory, LOCK_EX};
    const auto current = load_checkpoint_unlocked(directory);
    if (!current || current->device != checkpoint.device ||
        current->inode != checkpoint.inode ||
        current->offset != checkpoint.offset ||
        current->discard_until_newline != checkpoint.discard_until_newline ||
        current->request_file_sizes != checkpoint.request_file_sizes) {
        throw std::runtime_error("monitor checkpoint changed unexpectedly");
    }

    MonitorCheckpoint next = checkpoint;
    append_request_entries_unlocked(directory, entries, next);
    next.device = device;
    next.inode = inode;
    next.offset = offset;
    next.discard_until_newline = discard_until_newline;
    save_checkpoint_unlocked(directory, next);

    std::set<std::string> touched_dates;
    for (const AccessLogEntry& entry : entries) {
        touched_dates.insert(entry.timestamp.substr(0, 10));
    }
    write_summary_dates_unlocked(updated_statistics, directory, touched_dates);
    return next;
}

} // namespace subsite_monitor
