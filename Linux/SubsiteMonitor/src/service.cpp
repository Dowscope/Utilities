#include "service.hpp"

#include "collector.hpp"
#include "report.hpp"
#include "statistics.hpp"
#include "storage.hpp"

#include <algorithm>
#include <array>
#include <cerrno>
#include <charconv>
#include <chrono>
#include <csignal>
#include <cstdint>
#include <filesystem>
#include <iostream>
#include <limits>
#include <map>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <thread>
#include <utility>
#include <vector>

#include <fcntl.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <unistd.h>

namespace subsite_monitor {
namespace {

constexpr std::size_t max_batch_bytes = 1024U * 1024U;
constexpr std::size_t max_directory_entries = 4096U;
constexpr auto poll_interval = std::chrono::seconds{1};

volatile std::sig_atomic_t stop_requested = 0;

void signal_handler(int signal_number) {
    (void)signal_number;
    stop_requested = 1;
}

class SignalHandlers {
public:
    SignalHandlers() {
        struct sigaction action {};
        action.sa_handler = signal_handler;
        sigemptyset(&action.sa_mask);
        action.sa_flags = 0;
        if (sigaction(SIGTERM, &action, &old_term_) != 0 ||
            sigaction(SIGINT, &action, &old_int_) != 0) {
            (void)sigaction(SIGTERM, &old_term_, nullptr);
            throw std::runtime_error("cannot register service shutdown signals");
        }
        stop_requested = 0;
    }

    SignalHandlers(const SignalHandlers&) = delete;
    SignalHandlers& operator=(const SignalHandlers&) = delete;

    ~SignalHandlers() {
        (void)sigaction(SIGTERM, &old_term_, nullptr);
        (void)sigaction(SIGINT, &old_int_, nullptr);
    }

private:
    struct sigaction old_term_ {};
    struct sigaction old_int_ {};
};

class ServiceLock {
public:
    explicit ServiceLock(const std::filesystem::path& directory) {
        struct stat directory_status {};
        const bool existed = lstat(directory.c_str(), &directory_status) == 0;
        if (!existed && errno != ENOENT) {
            throw std::runtime_error("cannot inspect statistics directory: " +
                                     directory.string());
        }
        std::error_code error;
        std::filesystem::create_directories(directory, error);
        if (error) {
            throw std::runtime_error("cannot create statistics directory: " +
                                     error.message());
        }
        if (lstat(directory.c_str(), &directory_status) != 0 ||
            !S_ISDIR(directory_status.st_mode) ||
            directory_status.st_uid != geteuid()) {
            throw std::runtime_error(
                "statistics directory must be a real directory owned by this user");
        }
        if (existed && (directory_status.st_mode & 0077) != 0) {
            throw std::runtime_error(
                "existing statistics directory must already be private");
        }
        if (!existed && chmod(directory.c_str(), S_IRWXU) != 0) {
            throw std::runtime_error(
                "cannot restrict statistics directory permissions: " +
                directory.string());
        }
        const std::filesystem::path path = directory / ".service.lock";
        descriptor_ = open(path.c_str(), O_CREAT | O_RDWR | O_CLOEXEC |
                                               O_NOFOLLOW,
                           S_IRUSR | S_IWUSR);
        if (descriptor_ < 0) {
            throw std::runtime_error("cannot open monitor service lock: " +
                                     path.string());
        }
        struct stat status {};
        if (fstat(descriptor_, &status) != 0 || !S_ISREG(status.st_mode) ||
            status.st_uid != geteuid() || (status.st_mode & 0077) != 0) {
            (void)close(descriptor_);
            descriptor_ = -1;
            throw std::runtime_error("monitor service lock must be a private file");
        }
        if (flock(descriptor_, LOCK_EX | LOCK_NB) != 0) {
            (void)close(descriptor_);
            descriptor_ = -1;
            throw std::runtime_error("another monitor service is already running");
        }
    }

    ServiceLock(const ServiceLock&) = delete;
    ServiceLock& operator=(const ServiceLock&) = delete;

    ~ServiceLock() {
        if (descriptor_ >= 0) {
            (void)flock(descriptor_, LOCK_UN);
            (void)close(descriptor_);
        }
    }

private:
    int descriptor_{-1};
};

class FileDescriptor {
public:
    explicit FileDescriptor(int descriptor = -1) : descriptor_(descriptor) {}
    FileDescriptor(const FileDescriptor&) = delete;
    FileDescriptor& operator=(const FileDescriptor&) = delete;
    FileDescriptor(FileDescriptor&& other) noexcept
        : descriptor_(std::exchange(other.descriptor_, -1)) {}
    FileDescriptor& operator=(FileDescriptor&& other) noexcept {
        if (this != &other) {
            reset();
            descriptor_ = std::exchange(other.descriptor_, -1);
        }
        return *this;
    }
    ~FileDescriptor() { reset(); }

    [[nodiscard]] int get() const { return descriptor_; }
    void reset(int descriptor = -1) {
        if (descriptor_ >= 0) {
            (void)close(descriptor_);
        }
        descriptor_ = descriptor;
    }

private:
    int descriptor_{-1};
};

struct FileIdentity {
    std::uint64_t device{};
    std::uint64_t inode{};
};

FileIdentity identity_of(const struct stat& status) {
    return {static_cast<std::uint64_t>(status.st_dev),
            static_cast<std::uint64_t>(status.st_ino)};
}

bool same_identity(const FileIdentity& identity,
                   const MonitorCheckpoint& checkpoint) {
    return identity.device == checkpoint.device &&
           identity.inode == checkpoint.inode;
}

FileDescriptor open_log_file(const std::filesystem::path& path,
                             struct stat& status) {
    const int descriptor = open(path.c_str(), O_RDONLY | O_CLOEXEC | O_NOFOLLOW);
    if (descriptor < 0) {
        throw std::runtime_error("cannot open access log: " + path.string());
    }
    FileDescriptor result{descriptor};
    if (fstat(descriptor, &status) != 0 || !S_ISREG(status.st_mode) ||
        status.st_size < 0) {
        throw std::runtime_error("access log is not a readable regular file: " +
                                 path.string());
    }
    return result;
}

std::optional<std::filesystem::path> find_file_identity(
    const std::filesystem::path& active_path,
    const FileIdentity& identity) {
    const std::filesystem::path directory =
        active_path.has_parent_path() ? active_path.parent_path() : ".";
    std::error_code error;
    std::size_t count = 0;
    for (std::filesystem::directory_iterator iterator{directory, error}, end;
         !error && iterator != end; iterator.increment(error)) {
        if (++count > max_directory_entries) {
            throw std::runtime_error("access-log directory has too many files");
        }
        struct stat status {};
        if (lstat(iterator->path().c_str(), &status) != 0 ||
            !S_ISREG(status.st_mode)) {
            continue;
        }
        if (identity_of(status).device == identity.device &&
            identity_of(status).inode == identity.inode) {
            return iterator->path();
        }
    }
    if (error) {
        throw std::runtime_error("cannot scan access-log directory: " +
                                 error.message());
    }
    return std::nullopt;
}

std::optional<unsigned> numeric_rotation_suffix(
    const std::filesystem::path& path,
    const std::filesystem::path& active_path) {
    const std::string prefix = active_path.filename().string() + ".";
    const std::string name = path.filename().string();
    if (!name.starts_with(prefix) || name.size() == prefix.size()) {
        return std::nullopt;
    }
    unsigned suffix = 0;
    const char* first = name.data() + prefix.size();
    const char* last = name.data() + name.size();
    const auto [end, error] = std::from_chars(first, last, suffix);
    if (error != std::errc{} || end != last || suffix == 0U) {
        return std::nullopt;
    }
    return suffix;
}

std::vector<std::filesystem::path> rotation_successors(
    const std::filesystem::path& previous,
    const std::filesystem::path& active_path) {
    const auto suffix = numeric_rotation_suffix(previous, active_path);
    if (!suffix) {
        throw std::runtime_error(
            "rotated access log does not use numeric logrotate names; "
            "cannot safely catch up after rotation");
    }
    if (*suffix > max_directory_entries) {
        throw std::runtime_error("rotated access-log suffix is outside supported limits");
    }
    std::vector<std::filesystem::path> paths;
    for (unsigned number = *suffix; number > 1U;) {
        --number;
        const std::filesystem::path next =
            previous.parent_path() /
            (active_path.filename().string() + "." + std::to_string(number));
        struct stat next_status {};
        if (lstat(next.c_str(), &next_status) != 0 ||
            !S_ISREG(next_status.st_mode)) {
            throw std::runtime_error(
                "a rotated access log needed for recovery is missing: " +
                next.string());
        }
        paths.push_back(next);
    }
    paths.push_back(active_path);
    return paths;
}

void accumulate_request(StatisticsReport& report,
                        const AccessLogEntry& entry,
                        std::size_t& daily_bucket_count) {
    constexpr std::size_t max_sites = 10000U;
    constexpr std::size_t max_buckets = 100000U;
    auto position = report.sites.find(entry.hostname);
    if (position == report.sites.end()) {
        if (report.sites.size() >= max_sites) {
            throw std::runtime_error("statistics limit exceeded: too many websites");
        }
        position = report.sites.emplace(entry.hostname, SiteStatistics{}).first;
    }
    SiteStatistics& site = position->second;
    if (site.requests == std::numeric_limits<std::uint64_t>::max()) {
        throw std::runtime_error("statistics request count overflow");
    }
    ++site.requests;
    if (site.requests == 1 || entry.timestamp_utc > site.latest_access_utc) {
        site.latest_access_utc = entry.timestamp_utc;
    }
    const std::string date = entry.timestamp.substr(0, 10);
    auto day = site.requests_by_day.find(date);
    if (day == site.requests_by_day.end()) {
        if (daily_bucket_count >= max_buckets) {
            throw std::runtime_error("statistics limit exceeded: too many site-days");
        }
        day = site.requests_by_day.emplace(date, 0).first;
        ++daily_bucket_count;
    }
    if (day->second == std::numeric_limits<std::uint64_t>::max()) {
        throw std::runtime_error("daily request count overflow");
    }
    ++day->second;
    auto latest = site.latest_access_by_day.find(date);
    if (latest == site.latest_access_by_day.end() ||
        entry.timestamp_utc > latest->second) {
        site.latest_access_by_day[date] = entry.timestamp_utc;
    }
}

void service_loop(const std::filesystem::path& log_path,
                  const std::filesystem::path& statistics_directory,
                  FileDescriptor& current_log, struct stat current_status,
                  MonitorCheckpoint checkpoint,
                  StatisticsReport statistics) {
    std::size_t daily_bucket_count = 0;
    for (const auto& [hostname, site] : statistics.sites) {
        (void)hostname;
        daily_bucket_count += site.requests_by_day.size();
    }

    std::vector<std::filesystem::path> pending_rotations;
    std::size_t rotation_idle_polls = 0;
    while (stop_requested == 0) {
        const FileIdentity active_identity = identity_of(current_status);
        const std::uint64_t file_size =
            static_cast<std::uint64_t>(current_status.st_size);
        if (file_size < checkpoint.offset) {
            std::clog << "subsite-monitor service: access log truncated; "
                         "resuming at byte zero\n";
            checkpoint = commit_monitor_batch(
                statistics_directory, checkpoint, active_identity.device,
                active_identity.inode, 0, false, {}, statistics);
        }

        std::vector<AccessLogEntry> entries;
        const LogBatch batch = scan_log_batch(
            current_log.get(), checkpoint.offset, max_batch_bytes,
            checkpoint.discard_until_newline,
            [&entries, &statistics, &daily_bucket_count](
                const AccessLogEntry& entry) {
                accumulate_request(statistics, entry, daily_bucket_count);
                entries.push_back(entry);
            });
        if (batch.next_offset != checkpoint.offset ||
            batch.discard_until_newline != checkpoint.discard_until_newline) {
            checkpoint = commit_monitor_batch(
                statistics_directory, checkpoint, active_identity.device,
                active_identity.inode, batch.next_offset,
                batch.discard_until_newline, entries, statistics);
            if (batch.summary.parsed_records > 0U ||
                batch.summary.malformed_lines > 0U) {
                std::clog << "subsite-monitor service: recorded "
                          << batch.summary.parsed_records << " requests; "
                          << batch.summary.malformed_lines
                          << " malformed lines\n";
            }
        }

        struct stat path_status {};
        if (stat(log_path.c_str(), &path_status) != 0 ||
            !S_ISREG(path_status.st_mode)) {
            throw std::runtime_error("configured access log path disappeared: " +
                                     log_path.string());
        }
        const FileIdentity path_identity = identity_of(path_status);
        if (pending_rotations.empty() &&
            (path_identity.device != checkpoint.device ||
             path_identity.inode != checkpoint.inode)) {
            const auto previous = find_file_identity(
                log_path, {checkpoint.device, checkpoint.inode});
            if (!previous) {
                throw std::runtime_error(
                    "checkpointed access log was rotated out before it was read; "
                    "restore the uncompressed rotated log or reset the checkpoint");
            }
            pending_rotations = rotation_successors(*previous, log_path);
            rotation_idle_polls = 0;
            std::clog << "subsite-monitor service: access log rotation detected\n";
        }

        if (!pending_rotations.empty()) {
            struct stat descriptor_status {};
            if (fstat(current_log.get(), &descriptor_status) != 0) {
                throw std::runtime_error("cannot inspect monitored access log");
            }
            if (checkpoint.offset >=
                static_cast<std::uint64_t>(descriptor_status.st_size)) {
                ++rotation_idle_polls;
            } else {
                rotation_idle_polls = 0;
            }
            if (rotation_idle_polls >= 2U) {
                const std::filesystem::path next_path = pending_rotations.front();
                pending_rotations.erase(pending_rotations.begin());
                struct stat next_status {};
                FileDescriptor next_log = open_log_file(next_path, next_status);
                const FileIdentity next_identity = identity_of(next_status);
                checkpoint = commit_monitor_batch(
                    statistics_directory, checkpoint, next_identity.device,
                    next_identity.inode, 0, false, {}, statistics);
                current_log = std::move(next_log);
                current_status = next_status;
                rotation_idle_polls = 0;
            }
        } else if (fstat(current_log.get(), &current_status) != 0) {
            throw std::runtime_error("cannot inspect monitored access log");
        }

        std::this_thread::sleep_for(poll_interval);
    }
}

} // namespace

int run_monitor_service(const std::filesystem::path& log_path,
                        const std::filesystem::path& statistics_directory) {
    ServiceLock service_lock{statistics_directory};
    SignalHandlers signal_handlers;
    struct stat log_status {};
    FileDescriptor log_file = open_log_file(log_path, log_status);
    const FileIdentity identity = identity_of(log_status);

    std::optional<MonitorCheckpoint> loaded =
        load_monitor_checkpoint(statistics_directory);
    MonitorCheckpoint checkpoint;
    StatisticsReport statistics;
    if (!loaded) {
        bool discard_partial = false;
        if (log_status.st_size > 0) {
            char last_byte = '\0';
            const ssize_t count = pread(log_file.get(), &last_byte, 1,
                                        log_status.st_size - 1);
            if (count != 1) {
                throw std::runtime_error("cannot inspect end of access log");
            }
            discard_partial = last_byte != '\n';
        }
        checkpoint = initialize_monitor_checkpoint(
            statistics_directory, identity.device, identity.inode,
            static_cast<std::uint64_t>(log_status.st_size), discard_partial);
        statistics = recover_monitor_store(statistics_directory, checkpoint);
        std::clog << "subsite-monitor service: started at current log end; "
                     "new requests will be recorded\n";
    } else {
        checkpoint = *loaded;
        statistics = recover_monitor_store(statistics_directory, checkpoint);
        std::clog << "subsite-monitor service: resumed at byte "
                  << checkpoint.offset << '\n';
    }

    if (same_identity(identity, checkpoint)) {
        if (static_cast<std::uint64_t>(log_status.st_size) < checkpoint.offset) {
            checkpoint = commit_monitor_batch(
                statistics_directory, checkpoint, identity.device,
                identity.inode, 0, false, {}, statistics);
            std::clog << "subsite-monitor service: detected truncation; "
                         "resuming at byte zero\n";
        }
        service_loop(log_path, statistics_directory, log_file, log_status,
                     std::move(checkpoint), std::move(statistics));
    } else {
        const auto previous = find_file_identity(
            log_path, {checkpoint.device, checkpoint.inode});
        if (!previous) {
            throw std::runtime_error(
                "checkpointed access log was rotated out before restart; "
                "restore the uncompressed rotated log or reset the checkpoint");
        }
        FileDescriptor old_log = open_log_file(*previous, log_status);
        MonitorCheckpoint resumed = checkpoint;
        // The service loop drains the checkpointed file, then retained numeric
        // rotations, and finally the current active log.
        service_loop(log_path, statistics_directory, old_log, log_status,
                     std::move(resumed), std::move(statistics));
    }
    std::clog << "subsite-monitor service: stopped\n";
    return 0;
}

} // namespace subsite_monitor
