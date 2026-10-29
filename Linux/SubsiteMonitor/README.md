# Subsite Monitor

Current development version: **0.1.0-alpha.9**. The latest published release
is **0.1.0-alpha.8**. Release binaries target x86_64 Linux on
Debian, Ubuntu, and Arch, statically link ncursesw and the C++ runtime, and use
host terminfo for the terminal UI.

A small C++ command-line utility for monitoring website activity from web-server
access logs. It streams the documented Nginx log format, stores private daily
CSV statistics, and provides an ncurses screen with per-site summaries and request-level
history. Its default UI uses a root-only access-log configuration.

## Build and test

Requirements: GCC with C++23 support, GNU Make, `pkg-config`, and ncursesw
development files.

```sh
make -j
make test
```

Use `make clean` to remove build outputs, `make debug` for a debug build, and
`make install` to install under `/usr/local`, install the update helper under
`/usr/share/subsite-monitor`, and seed the root-only config at
`/etc/subsite-monitor/monitor.conf` (override `PREFIX`, `SYSCONFDIR`, or use
`DESTDIR` for packaging).

For local development, `./run-dev.sh` builds and opens the ncurses UI with the
bundled sample log and Nginx config. Pass arguments to run another command.
Run `./verify.sh` for a clean build, tests, and CLI smoke checks.

For a live Nginx test, run `./run-docker.sh` in one terminal. In another,
generate requests with `curl` and run `./run-dev.sh`; it automatically uses the
Docker-shared config and log when present. See the [manual](Docs/MANUAL.md) for
exact commands and how to point the app at reviewed copies of server data.
The [user and technical manuals](Docs/README.md) describe current behavior and
design. [Prompt.md](Prompt.md) preserves the project handoff brief,
[Changelog.md](Changelog.md) records changes, and [TODO.md](TODO.md) tracks
approved future work.

The UI site table opens with most recently active sites first and supports
sorting by website, request count, or last access with keys `1`–`3`; repeat the
active key to reverse the direction. The UI command bars use grouped
`Action: key | Action: key` hints; update availability appears in the lower
right only when a newer compatible release is found. The alpha.9 source and
release archive are built and verified. The archive checksum is in
`packaging/dist/SHA256SUMS`; see `packaging/README.md` for the release title,
notes, and upload steps.
See `packaging/README.md` for alpha build and release steps.

## Current CLI

```text
subsite-monitor --help
subsite-monitor --version
sudo subsite-monitor
subsite-monitor collect --log /path/to/access.log
subsite-monitor stats
subsite-monitor stats --days 7
subsite-monitor stats --log /path/to/access.log
sudo subsite-monitor service
sudo subsite-monitor ui
subsite-monitor ui --log /path/to/access.log
```

`collect` prints parsed and malformed line counts and writes daily summary
CSVs plus a daily event CSV containing every valid request. `stats` reads the
summary snapshots; `--days N` limits output to the newest N dates with stored
data. The UI shows each stored request, including its full URL and client IP.
See the [manual](Docs/MANUAL.md) for storage details and limits.

`service` follows the configured log in the foreground and appends new request
rows and summary counts to the CSV store. `make install` also installs a
systemd unit; it does not enable or start it. See the manual for the initial
backfill choice, systemd commands, and rotation requirements.

With no arguments, the application opens the interactive UI using the
root-only configuration. It defaults to
`/var/log/subsite-monitor/access.log`; edit the config with
`sudoedit /etc/subsite-monitor/monitor.conf`. The log must use the documented
host-aware Nginx format for per-site activity. The UI reads `server_name`
directives from the configured Nginx config and its includes, then merges those
names with hostnames observed in the log. Sites with no requests appear with
zero requests and `Never` as the last access. Literal and wildcard names are
listed; regex names and `_` are skipped. Use `SUBSITE_MONITOR_LOG` and
`SUBSITE_MONITOR_NGINX_CONFIG` to override paths, or `SUBSITE_MONITOR_CONFIG`
to select another root-only config.

The `ui` command uses the same configuration, or accepts `--log <path>` as an
override. Use the arrow keys or `j`/`k` to select a site. Enter opens every stored
request for that site, starting with the newest 100 requests. The site overview
defaults to latest access first; keys `1`–`3` sort by website, request count,
and last access. Repeat a key to reverse the sort direction. The history table includes
server-local timestamp with offset, client IP, method, status, and full request
target (path and query). Press `1`–`4` to sort the current page by time, IP,
method/status, or URL; repeat the active key to reverse direction. Up/down and
Page Up/Down move through requests/pages, left/right inspect long targets, and
Escape returns to the overview. The site name is shown above; the current log
format does not record the scheme. Press `r` to reread the log and `q` to quit.
Sorting is limited to one 100-record page to keep memory bounded. The UI
requires an interactive terminal. Press `u` in the site overview to check for
newer compatible releases, review release notes, and confirm installation. The
updater uses Python 3 to read GitHub release metadata and requires network
access. It runs the installed checksum-verifying installer, invoking `sudo`
when needed. Installation can reload Nginx and restart the monitor service.
Quit and relaunch the UI after an update. For sample data, run:

```sh
./run-dev.sh
```

With no arguments, `run-dev.sh` builds and opens the UI, using Docker-generated
Nginx data when available and fixture data otherwise. Any arguments are passed
to `subsite-monitor`, allowing CLI commands or a different log path.

## Alpha installer

The installer and release build instructions are in `installer.sh` and
`packaging/README.md`. Alpha.8 is the latest published binary; alpha.9's
archive and checksum are ready for its GitHub Release. Release
binaries target Debian, Ubuntu, and Arch on x86_64.
The installer seeds the root-only monitor config and installs a host-aware
snippet at `/etc/nginx/conf.d/subsite-monitor.conf`, a logrotate policy, and the
systemd unit. The snippet loads when the main Nginx config includes
`conf.d/*.conf` inside `http { }`. The installer checks that include, tests the
Nginx configuration, and reloads `nginx.service` when it is active. If the
configured log is readable, it reloads systemd and enables/starts the monitor
service; on upgrades it restarts an active monitor. It does not edit the main
Nginx config or start an inactive Nginx service. The default monitored log is
`/var/log/subsite-monitor/access.log`. The installer lists up to 10 compatible
published releases and verifies the selected archive's SHA-256. Use
`--list`, `--latest`, or `--version <version>` to control selection; remote
installation requires Python 3 and curl or wget.
