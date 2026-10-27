# Subsite Monitor Technical Reference

## Current architecture

The executable has six translation units: `src/main.cpp` handles CLI
arguments, `src/collector.cpp` streams log files, `src/log_parser.cpp`
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

The interactive screen requires ncursesw development headers and library plus
`pkg-config` metadata for `ncursesw`. The executable links with `-lncursesw`.

The default install prefix is `/usr/local`, with the executable installed in
`bin`. `DESTDIR` prepends a staging root to the install destination. No install
step is needed to run the binary from the project directory.

## CLI and process behavior

- `--help` and `-h`: print usage and return 0.
- `--version` and `-V`: print `subsite-monitor 0.1.0` and return 0.
- `collect --log <path>`: parse a supported access log; print aggregate valid
  and malformed line counts only.
- `stats --log <path>`: scan one log and print per-site and daily statistics.
- `ui --log <path>`: load one statistics snapshot and display it in ncurses.
- No arguments: print help and return 0.
- Invalid command arguments return 2. Log open/read failures return 1.

The program reads only the explicitly supplied log path. It performs no
network access and does not write state.

At this stage `stats` requires `--log`; persistent history and configuration
are future work, so a bare `stats` command cannot select an input yet.

The TUI requires both stdin and stdout to be terminals. It reads one snapshot
at startup; `r` reruns statistics from the same path. Arrow keys and `j`/`k`
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
`tests/test_statistics.cpp` checks aggregation, ordering, timestamp normalization,
and redaction. `tests/test_cli.sh` checks command behavior, sample log
collection, oversized lines, resource caps, and error exits, including UI
behavior when no TTY is available. `make test` runs all three. `verify.sh`
runs `make clean`, builds in parallel, runs the suite, and checks help/version.
It passed on 2026-10-27T12:20:26-04:00. The no-argument `run-dev.sh` UI launch
was also exercised in a pseudo-terminal.

## Security and operational constraints

Log contents are treated as untrusted input. Parsing uses a fixed-size line
buffer and validates field syntax and lengths. The collector only reads its
input file and reports open/read failures to the caller; it does not modify the
web server. The command does not print sensitive URL query parameters or client
IPs. The TUI sanitizes control characters in displayed file names and error
messages. Client IP attribution through a reverse proxy requires trusted proxy
configuration. The application does not require root when filesystem
permissions allow access to logs and its own state directories.

Checkpoint durability, CSV write safety, lock behavior, alternative log
formats, and configuration semantics are not defined by the current
implementation.
Document those specifics alongside the phase that introduces them.

Last reviewed: 2026-10-27T12:20:44-04:00. This reference describes the current
CLI and TUI.
