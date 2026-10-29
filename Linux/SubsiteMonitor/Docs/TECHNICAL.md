# Subsite Monitor Technical Reference

## Current architecture

The executable has ten translation units: `src/main.cpp` handles CLI
arguments, `src/config.cpp` parses and secures the root config,
`src/nginx_config.cpp` discovers configured virtual-host names,
`src/collector.cpp` streams log files, `src/log_parser.cpp`
validates individual records, `src/statistics.cpp` aggregates them by host and
log date, `src/report.cpp` formats text output, and `src/tui.cpp` renders the
interactive screen with ncursesw. Their interfaces are declared in
corresponding headers. `src/storage.cpp` implements daily CSV persistence,
checkpoint recovery, directory locking, atomic replacement, and history reads.
`src/service.cpp` implements the foreground log follower and rotation handling.

The terminal report and TUI share the statistics model. `src/report.cpp`
provides text formatting plus UTC storage and local-time display formatting; `src/tui.cpp` manages
ncurses screen lifecycle, input, and layout.

## Build details

The Makefile uses `g++`, `-std=c++23`, `-O2`, and the warning flags
`-Wall -Wextra -Wpedantic -Wconversion`. Compilation emits `.d` dependency
files through `-MMD -MP`, so header changes trigger rebuilds. Parallel jobs are
enabled by invoking GNU Make with `-j` (for example `make -j`).

Development builds require ncursesw development headers and library plus
`pkg-config` metadata for `ncursesw`. Alpha release packaging builds pinned
ncurses 6.5 static libraries and statically links the executable. The release
binary still uses the host terminfo database for terminal capabilities.

The default install prefix is `/usr/local`, with the executable installed in
`bin`. `make install` also seeds `/etc/subsite-monitor/monitor.conf` as mode
0600 under a mode 0700 directory, preserving an existing config, and installs
`systemd/subsite-monitor.service` under `/usr/lib/systemd/system`. It does not
enable or start the unit. It also creates `/var/lib/subsite-monitor` mode 0700
for service state. `STATEDIR` changes that path, and `DESTDIR` prepends a
staging root to install paths. It creates `/var/log/subsite-monitor` mode 0750,
installs the Nginx snippet to `/etc/nginx/conf.d/subsite-monitor.conf`, and
installs a separate `/etc/logrotate.d/subsite-monitor` policy. The snippet is
automatically loaded when the main Nginx config includes `conf.d/*.conf` from
inside `http { }`; the installer does not edit the main config. The release
installer tests Nginx, checks that the snippet appears in `nginx -T`, and
reloads `nginx.service` when active. It reloads systemd and enables/starts the
monitor service when the configured log is readable, and restarts an active
monitor on upgrade. It skips service activation if systemd or the log is
unavailable. Existing snippets and logrotate files are preserved.

## CLI and process behavior

- `--help` and `-h`: print usage and return 0.
- `--version` and `-V`: print `subsite-monitor 0.1.0-alpha.7` and return 0.
- `collect --log <path> [--stats-dir <path>]`: scan a supported access log,
  report valid/malformed counts, and replace daily CSV snapshots.
- `stats [--days N] [--stats-dir <path>]`: read stored daily CSVs; `--days`
  selects the newest N dates containing stored data.
- `stats --log <path>`: scan one log and print a one-time report.
- `service`: follow the configured log in the foreground and persist appended
  event rows and updated summaries. Intended to run under systemd.
- `ui --log <path>`: scan, store, and display statistics; Enter opens the
  selected website's daily history.
- `ui` with no options: read the default root-only config and launch ncurses.
- No arguments: launch the same UI as `ui`.
- Invalid command arguments return 2. Log open/read failures return 1.

The default config path is `/etc/subsite-monitor/monitor.conf`; supported
fields are absolute `access_log` and `nginx_config` paths. The latter defaults
to `/etc/nginx/nginx.conf`. `SUBSITE_MONITOR_CONFIG` selects another config
file, while `SUBSITE_MONITOR_LOG` and `SUBSITE_MONITOR_NGINX_CONFIG` override
individual paths. The CLI `--log` option takes precedence for that command.
The program performs no network access. Collection writes CSV state as
described below.

The release installer accepts `SUBSITE_MONITOR_ARCHIVE=/path/to/archive` for
offline testing. It applies the same pinned SHA-256 check as a downloaded
archive. `SUBSITE_MONITOR_ACCESS_LOG` changes the default log path at install;
the installer renders that path into a new monitor config, Nginx snippet, and
logrotate policy. On upgrade it migrates only the previous built-in log path;
custom configured paths are preserved. Log rotation is daily, retains 14
rotations, delays compression, and signals the Nginx systemd unit to reopen the
log. Nginx access-log directives defined at a deeper `server` or `location`
level override the shared `http`-level log.
The installer stores its own version in `APP_VERSION` so sourcing
`/etc/os-release` on Debian cannot overwrite the package version used by the
executable compatibility check.

For remote installs, `installer.sh` queries the public GitHub Releases API,
filters for up to 10 non-draft releases with the expected x86_64 archive,
`SHA256SUMS`, and a valid SHA-256 archive digest, then offers an interactive
version menu. Non-interactive runs select the newest compatible release.
`--latest`, `--list`, and `--version <version>` support unattended, inspection,
and pinned installs. Python 3 parses the API JSON; curl or wget downloads the
metadata and archive. Release tags are validated before use in download URLs.

The statistics directory defaults to `/var/lib/subsite-monitor/stats` for
root and to the XDG state path for regular users. `SUBSITE_MONITOR_STATS_DIR`
or `--stats-dir` overrides it. The directory is mode 0700 and must be owned by
the effective user; lock and CSV files are mode 0600. An advisory `flock`
serializes access. Each daily CSV is written to a same-directory temporary
file, synced, renamed atomically, and followed by a directory sync. A run
replaces only dates seen in that scan, making repeated scans of a complete log
idempotent. It also writes `requests-YYYY-MM-DD.csv` event files with schema
`timestamp_utc,site,client_ip,method,url,status`. Event rows are streamed to
temporary files by log date with at most 64 request files open; each file is
synced and atomically renamed after a successful scan. Request reads stream the
files and return bounded pages, keeping event-history memory bounded. Full URLs
and client IPs are sensitive and are protected by the private directory/file
modes. Disk usage grows with logged request volume. Manual collection is a full
snapshot operation. The `service` command instead uses a private
`.monitor-checkpoint` containing the current log device, inode, byte offset,
incomplete-line state, and committed request CSV sizes. It appends and syncs
event rows before atomically advancing the checkpoint. Startup trims
uncommitted event tails, removes uncommitted new event files, rebuilds summaries
from event records, and then resumes. A separate `.service.lock` prevents two
services and full snapshot collection from modifying the same store at once.
The service reads at most 1 MiB per polling batch and retains only that batch's
entries plus aggregate statistics in memory.

With no checkpoint, service startup records the active log's current end as its
initial cursor. Historical backfill is an explicit one-time `collect` before
starting the service. Incomplete lines are not processed until newline; an
incomplete preexisting tail at first start is skipped. Rename rotation supports
retained, uncompressed numeric suffixes, draining the old descriptor before
following newer files. Missing or compressed rotated inputs fail visibly.
Truncation to a smaller size resets the cursor to zero, but copytruncate may
lose records when the file is truncated and regrows past the cursor between
one-second polls.

The Nginx config scanner follows `include` directives, including filename
wildcards, and extracts `server_name` values in `server` blocks. It bounds
file size, total scanned bytes, file count, nesting depth, token size, and
discovered sites. Literal and wildcard names are normalized to lowercase and
deduplicated; regex names and `_` are skipped. The UI merges this inventory
with hosts seen in logs, showing configured sites with no activity as zero.

When Nginx config discovery is enabled but the configured access log cannot
be opened, the TUI still displays the discovered site inventory and reports
that activity is unavailable. Other log processing errors remain fatal.

The TUI requires both stdin and stdout to be terminals. When the service lock is
active it reads the persisted CSV snapshots; otherwise startup and `r` perform
a full collection and reload stored statistics. Arrow keys and
`j`/`k` change the selected site. Enter opens that site's request records;
Each request is rendered as one table row with its timestamp converted from
UTC to the monitor host's local timezone and shown with a numeric offset.
The initial request page is the newest 100 events, ordered newest first.
Up/Down moves within that page, Page Up/Down loads adjacent chronological
pages, and Left/Right horizontally scrolls the selected full URL. Keys `1` to
`4` sort the current page by timestamp, client IP, method/status, or URL; pressing
the active key again reverses its direction. Sorting is page-local to retain a
bounded 100-record working set. Escape returns to the
overview and `q` exits. The screen is redrawn on
terminal resize. Ncursesw is linked through the `pkg-config ncursesw` metadata.

## Nginx parser contract

Supported format string:

```nginx
log_format subsite_monitor '$time_iso8601 $host $remote_addr "$request" $status';
```

The parser accepts the ISO-8601 timestamp shape emitted by `$time_iso8601`,
ASCII host/address tokens, a quoted Nginx request line, and a three-digit
status from 100 through 599. It validates calendar dates, time and offset
ranges, method tokens, request-target bytes, and the exact field order. It
recognizes Nginx default escaping (`\"`, `\\`, and `\xHH`) inside the quoted
request. It rejects unsupported escapes, controls in the decoded request
target, trailing fields, and overlong values.

The collector uses a fixed 16,384-byte line buffer. It consumes and marks an
overlong line malformed without retaining the excess bytes. Empty lines are
malformed records. A final line without a newline is still parsed. The
collection summary uses 64-bit counters. Client addresses and targets exist
only in the parsed record and are not printed or stored.

Statistics retain one aggregate per host and one daily count per distinct
host/date pair. Day grouping uses the date text in the access log. Latest
access comparisons normalize each timestamp offset to UTC. Statistics are stored
in UTC; text reports and the UI render those instants in the local timezone of
the running monitor and include the numeric offset. The report orders sites by request total
descending, with hostname as a deterministic tie-breaker. Its output contains
hostnames, counts, dates, and timestamps; summary output omits request targets
and client addresses. Aggregation is capped at 10,000 distinct hostnames and 100,000
host/date buckets to bound memory; hitting a cap aborts the report with an
error.

## Verification

`tests/test_parser.cpp` checks field extraction and malformed inputs.
`tests/test_statistics.cpp` checks aggregation, ordering, timestamp
normalization, and redaction. `tests/test_config.cpp` checks config parsing and
root-only access policy. `tests/test_cli.sh` checks command behavior, sample
log collection, oversized lines, resource caps, and UI event-history display.
`tests/test_storage.cpp` checks daily snapshots, event fields, full query strings,
CSV escaping, and request pagination. `tests/test_service.sh` checks live append,
incomplete lines, rotation, restart recovery, truncation, and concurrent
collector exclusion. `tests/test_installer_template.sh` builds a temporary
release archive and verifies the generated installer installs the executable,
configuration, state directory, and service unit without enabling the unit.
`make test` runs these suites. `verify.sh` runs a clean build, tests, and CLI
smoke checks. The default UI was also exercised in a pseudo-terminal using the
sample log and displayed 2 websites.

## Security and operational constraints

Log contents are treated as untrusted input. Parsing uses a fixed-size line
buffer and validates field syntax and lengths. The collector only reads its
input file and reports open/read failures to the caller; it does not modify the
web server. Summary commands do not print URL query parameters or client IPs. The request
history screen explicitly displays full URLs and client IPs. The UI replaces
control bytes before rendering stored/log-derived text. The default config must
be root-owned, readable by root only, and stored in a root-owned directory that
is not group/world writable. The default
UI therefore requires root; explicit `--log` commands remain available to
users with log read permission. Client IP attribution through a reverse proxy
requires trusted proxy configuration.

Known operational limits include the copytruncate truncate-and-regrow race,
catch-up support only for retained uncompressed numeric rotations, and the
single documented Nginx log format. Configurable statistics and checkpoint
directories in the root-only config remain future work.

The website table defaults to latest access descending. Keys `1` through `3`
sort by hostname, request count, or last access; pressing the active key toggles
direction. Request-history sorting applies to its current 100-record page.
Both tables render unselected rows at normal brightness and the selected row
in bold white on blue.

Last reviewed: 2026-10-29. This reference describes the CLI, TUI, root-only
log config, CSV storage, checkpointed service, Nginx integration, and alpha.7
development source. Alpha.6 is the latest published package.


## Docker Nginx development fixture

`compose.yaml` starts an Nginx container only; the Subsite Monitor development
binary is built and run natively on the host through `run-dev.sh`. The test
Nginx config and virtual-host files live under `docker/nginx/` and are mounted
read-only into the container. Nginx writes its host-aware access log to the
Git-ignored `docker-data/logs/access.log` bind mount so both the host app and
container see the same file. The service binds to localhost port 8080 by
default. Its container filesystem remains writable because the official Nginx
entrypoint prepares writable log and cache paths at startup.

For this host/container arrangement, `run-dev.sh` honors
`SUBSITE_MONITOR_LOG` and `SUBSITE_MONITOR_NGINX_CONFIG`. With no overrides it
uses the Docker-mounted Nginx config and log if the log file exists, otherwise
the checked-in parser fixtures. This lets developers create requests against a
real Nginx process while keeping app compilation and UI interaction on the
development host. It is a development fixture, not a production service.
