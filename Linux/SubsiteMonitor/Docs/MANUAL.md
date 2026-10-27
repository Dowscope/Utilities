# Subsite Monitor User Manual

## What the program does

Subsite Monitor helps identify which hosted websites receive requests by
reading existing web-server access logs. It reads the supported
Nginx access-log format, counts valid and malformed lines, and does not print
request targets or client IPs. It can also calculate per-site/daily statistics
and display those statistics in an ncurses terminal screen. The default UI
uses a root-only config file. CSV persistence is not yet implemented.

## Requirements

- Linux or another environment with GNU Make and GCC.
- A GCC version with C++23 support.
- GNU `pkg-config` and ncursesw development files for the terminal UI.

On Debian or Ubuntu, install `g++ make pkg-config libncurses-dev`. On Arch,
install `gcc make pkgconf ncurses`.

The alpha release installer supports x86_64 Debian, Ubuntu, and Arch systems.
Its binary statically links the runtime libraries; the terminal UI still needs
a usable terminfo database. The development build requirements above are not
needed to run the installed release executable.

## Build and test

From the project directory:

```sh
make -j
make test
```

`make clean` removes the executable and generated object/dependency files.
`make debug` builds with debug symbols and no optimization. `make install`
installs the executable under `/usr/local/bin` and seeds
`/etc/subsite-monitor/monitor.conf` with mode 0600. Set `PREFIX` or
`SYSCONFDIR` to change paths, or set `DESTDIR` to stage installation.

The helper scripts provide common local workflows:

- `./run-dev.sh` builds as needed and runs the program. With no arguments it
  opens the ncurses UI using `tests/fixtures/access.log`; pass CLI arguments
  to run a specific command or choose another log.
- `./verify.sh` performs a clean build, runs parser and CLI tests, and checks
  help and version output. It includes tests for config parsing and default UI
  dispatch.

## Commands currently available

```sh
subsite-monitor --help
subsite-monitor --version
sudo subsite-monitor
subsite-monitor collect --log /var/log/nginx/subsite-monitor-access.log
subsite-monitor stats --log /var/log/nginx/subsite-monitor-access.log
sudo subsite-monitor ui
subsite-monitor ui --log /var/log/nginx/subsite-monitor-access.log
```

With no arguments, the program opens the UI using the root-only config.
`collect` requires exactly `--log <path>` and prints parsed and malformed
record counts. Empty lines, invalid records, and lines over 16,384 bytes count
as malformed. An unreadable or missing file returns exit status 1. Unsupported
or incomplete arguments return status 2; help, version, and a completed
collection return status 0. Starting the UI without an interactive terminal
returns status 1.

`stats --log <path>` calculates a one-time report directly from that log. It
shows per-website request totals and latest access in UTC, daily counts grouped
by the date written in the log, and malformed-line count. Sites are sorted by
total requests, highest first. This version requires `--log`; it does not yet
read stored history or a configured default log path. To keep aggregation
memory bounded, one report accepts at most 10,000 distinct websites and
100,000 website/date pairs; exceeding either limit returns an error.

The installer creates `/etc/subsite-monitor/monitor.conf`, owned by root and
mode 0600, within a mode 0700 directory. Edit it with `sudoedit`. The default
`access_log` is `/var/log/nginx/subsite-monitor-access.log`. The no-argument
UI and `ui` command read this config, so use `sudo subsite-monitor` or
`sudo subsite-monitor ui`. `SUBSITE_MONITOR_CONFIG` selects another root-only
config, and `SUBSITE_MONITOR_LOG` overrides only the log path. `ui --log
<path>` overrides the config and can be used by a non-root user with read
permission.

The log must use the host-aware Nginx format below. The UI counts distinct
hostnames with valid requests in the log; a site with no valid request cannot
be discovered from access logs alone. The screen is a snapshot. Press up/down
or `j`/`k` to select a website and view daily counts, `r` to reread the log, and
`q` or Escape to exit. It requires an interactive terminal and omits full log
paths, request targets, and client addresses.

The config accepts one `access_log = /absolute/path` entry, ignores blank and
comment lines, and rejects unknown/duplicate keys, relative paths, control
characters, lines over 4,096 bytes, and files over 16 KiB. Reading the default
config requires effective root privileges. The program also verifies root
ownership and restrictive permissions before parsing it.

## Supported Nginx access-log format

Configure a custom log format in Nginx's `http` context and apply it to the
desired virtual hosts:

```nginx
log_format subsite_monitor '$time_iso8601 $host $remote_addr "$request" $status';
access_log /var/log/nginx/subsite-monitor-access.log subsite_monitor;
```

The format is one line per request, in this exact field order:

```text
ISO-8601 timestamp host client-IP "METHOD request-target HTTP-version" status
```

For example:

```text
2026-10-27T14:45:32+00:00 live.example.test 203.0.113.10 "GET / HTTP/1.1" 200
```

The parser accepts timestamps with a numeric `+HH:MM` or `-HH:MM` offset,
hostname characters `A-Z`, `a-z`, digits, dot, hyphen, and underscore, and
IPv4/IPv6-style client address characters. Methods are limited to 32 bytes and
request targets to 8,192 bytes. The full input line is limited to 16,384 bytes.
Nginx's default log escaping for quotes, backslashes, and control bytes in
`$request` is recognized. Other log formats are not auto-detected.

When Nginx is behind a reverse proxy, `$remote_addr` will normally be the proxy
address. Configure Nginx's real IP handling only for known, trusted proxy
addresses if the actual client address is required; do not trust arbitrary
client-supplied forwarded headers.

## Planned behavior

CSV history and incremental checkpoints are planned in later phases. See the
project [TODO](../TODO.md) for the phase list. The parser
retains the request target and client address only while reading each record;
`collect`, `stats`, and the UI do not display them. Later reports must continue
to avoid exposing sensitive query parameters.

Last reviewed: 2026-10-27T14:22:35-04:00. This manual describes the current
CLI and TUI.
