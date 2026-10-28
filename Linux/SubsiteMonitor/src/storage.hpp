#pragma once

#include "statistics.hpp"

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <map>
#include <optional>
#include <string>
#include <vector>

namespace subsite_monitor {

struct SiteDayHistory {
    std::string date;
    std::uint64_t requests{};
    std::chrono::sys_seconds latest_access_utc{};
};

struct RequestRecord {
    std::string date;
    std::string hostname;
    std::chrono::sys_seconds timestamp_utc{};
    std::string client_ip;
    std::string method;
    std::string url;
    std::uint16_t status{};
};

struct MonitorCheckpoint {
    std::uint64_t device{};
    std::uint64_t inode{};
    std::uint64_t offset{};
    bool discard_until_newline{};
    std::map<std::string, std::uint64_t> request_file_sizes;
};

struct RequestPage {
    std::vector<RequestRecord> records;
    std::size_t offset{};
    bool has_more{};
};

[[nodiscard]] std::filesystem::path default_statistics_directory();

// Scans the complete log, stores aggregate snapshots and every request row.
[[nodiscard]] StatisticsReport collect_and_store(
    const std::filesystem::path& log_path,
    const std::filesystem::path& directory);

// Reads all daily CSV files, or the most recent `days` stored dates.
[[nodiscard]] StatisticsReport read_stored_statistics(
    const std::filesystem::path& directory, std::size_t days = 0);

// Reads the selected site's rows from its daily CSV snapshots, newest first.
[[nodiscard]] std::vector<SiteDayHistory> read_site_history(
    const std::filesystem::path& directory, const std::string& hostname,
    std::size_t days = 0);

// Reads one bounded, chronological page from the selected site's event CSVs.
[[nodiscard]] RequestPage read_site_requests(
    const std::filesystem::path& directory, const std::string& hostname,
    std::size_t offset, std::size_t limit);

[[nodiscard]] bool monitor_service_is_active(
    const std::filesystem::path& directory);
[[nodiscard]] std::optional<MonitorCheckpoint> load_monitor_checkpoint(
    const std::filesystem::path& directory);
[[nodiscard]] MonitorCheckpoint initialize_monitor_checkpoint(
    const std::filesystem::path& directory, std::uint64_t device,
    std::uint64_t inode, std::uint64_t offset, bool discard_until_newline);
[[nodiscard]] StatisticsReport recover_monitor_store(
    const std::filesystem::path& directory,
    const MonitorCheckpoint& checkpoint);
[[nodiscard]] MonitorCheckpoint commit_monitor_batch(
    const std::filesystem::path& directory,
    const MonitorCheckpoint& checkpoint, std::uint64_t device,
    std::uint64_t inode, std::uint64_t offset, bool discard_until_newline,
    const std::vector<AccessLogEntry>& entries,
    const StatisticsReport& updated_statistics);

} // namespace subsite_monitor
