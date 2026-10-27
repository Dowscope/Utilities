# Subsite Monitor Technical Reference

## Current architecture

The executable has eight translation units: `src/main.cpp` handles CLI
arguments, `src/config.cpp` parses and secures the root config,
`src/nginx_config.cpp` discovers configured virtual-host names,
`src/collector.cpp` streams log files, `src/log_parser.cpp`
validates individual records, `src/statistics.cpp` aggregates them by host and
log date, `src/report.cpp` formats text output, and `src/tui.cpp` renders the
interactive screen with ncursesw. Their interfaces are declared in
corresponding headers.

The terminal report and TUI share the statistics model. `src/report.cpp`
provides text formatting and UTC timestamp formatting; `src/tui.cpp` manages
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
0600 under a mode 0700 directory, preserving an existing config. `DESTDIR`
prepends a staging root to both install destinations.

## CLI and process behavior

- `--help` and `-h`: print usage and return 0.
- `--version` and `-V`: print `subsite-monitor 0.1.0-alpha.3` and return 0.
- `collect --log <path>`: parse a supported access log; print aggregate valid
  and malformed line counts only.
- `stats --log <path>`: scan one log and print per-site and daily statistics.
- `ui --log <path>`: load one statistics snapshot and display it in ncurses.
- `ui` with no options: read the default root-only config and launch ncurses.
- No arguments: launch the same UI as `ui`.
- Invalid command arguments return 2. Log open/read failures return 1.

The default config path is `/etc/subsite-monitor/monitor.conf`; supported
fields are absolute `access_log` and `nginx_config` paths. The latter defaults
to `/etc/nginx/nginx.conf`. `SUBSITE_MONITOR_CONFIG` selects another config
file, while `SUBSITE_MONITOR_LOG` and `SUBSITE_MONITOR_NGINX_CONFIG` override
individual paths. The CLI `--log` option takes precedence for that command.
The program performs no network access and does not write persistent state.

At this stage `stats` and `collect` require `--log`; persistent history and
configuration for statistics/state directories remain future work.

The Nginx config scanner follows `include` directives, including filename
wildcards, and extracts `server_name` values in `server` blocks. It bounds
file size, total scanned bytes, file count, nesting depth, token size, and
discovered sites. Literal and wildcard names are normalized to lowercase and
deduplicated; regex names and `_` are skipped. The UI merges this inventory
with hosts seen in logs, showing configured sites with no activity as zero.

The TUI requires both stdin and stdout to be terminals. It reads one snapshot
at startup; `r` reruns statistics and config discovery. Arrow keys and `j`/`k`
change the selected site, while `q` or Escape exits. The screen is redrawn on
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
access comparisons normalize each timestamp offset to UTC, and the report
prints that latest instant in UTC. The report orders sites by request total
descending, with hostname as a deterministic tie-breaker. Its output contains
hostnames, counts, dates, and timestamps; it omits request targets and client
addresses. Aggregation is capped at 10,000 distinct hostnames and 100,000
host/date buckets to bound memory; hitting a cap aborts the report with an
error.

## Verification

`tests/test_parser.cpp` checks field extraction and malformed inputs.
`tests/test_statistics.cpp` checks aggregation, ordering, timestamp
normalization, and redaction. `tests/test_config.cpp` checks config parsing and
root-only access policy. `tests/test_cli.sh` checks command behavior, sample
log collection, oversized lines, resource caps, and UI dispatch without a TTY.
`make test` runs these suites. `verify.sh` runs a clean build, tests, and CLI
smoke checks. The default UI was also exercised in a pseudo-terminal using the
sample log and displayed 2 websites.

## Security and operational constraints

Log contents are treated as untrusted input. Parsing uses a fixed-size line
buffer and validates field syntax and lengths. The collector only reads its
input file and reports open/read failures to the caller; it does not modify the
web server. The command does not print sensitive URL query parameters or client
IPs. The TUI sanitizes control characters in displayed file names and error
messages. The default config must be root-owned, readable by root only, and
stored in a root-owned directory that is not group/world writable. The default
UI therefore requires root; explicit `--log` commands remain available to
users with log read permission. Client IP attribution through a reverse proxy
requires trusted proxy configuration.

Checkpoint durability, CSV write safety, lock behavior, alternative log
formats, and configuration for statistics/state directories remain undefined.

Last reviewed: 2026-10-27. This reference describes the CLI,
TUI, root-only log config, and next alpha release build.
