# Subsite Monitor

Current development version: **0.1.0-alpha.6**. The latest published release
is **0.1.0-alpha.5**. Release binaries target x86_64 Linux on
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
`make install` to install under `/usr/local` and seed the root-only config at
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

`./verify.sh` and `./packaging/build-alpha.sh` passed. The clean
C++23 build, parser/statistics/config/Nginx-config/storage/CLI/service and
installer-template tests passed without compiler warnings. The alpha.5 archive
and its checksum-pinned installers remain the latest published assets. The
alpha.6 source tag contains the table readability update but has no release
archive yet. Alpha.5 keeps configured sites visible if the selected access log
is unavailable.
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
request for that site, including server-local timestamp with offset, client IP, method, status, and
full request target (path and query). The site name is shown above; the current
log format does not record the scheme. Use up/down to navigate, left/right to
inspect long targets, Page
Up/Down to move through pages, and Escape to return. Press `r` to reread the log
and `q` to quit. It requires a real interactive terminal. For sample data,
run:

```sh
./run-dev.sh
```

With no arguments, `run-dev.sh` builds and opens the UI, using Docker-generated
Nginx data when available and fixture data otherwise. Any arguments are passed
to `subsite-monitor`, allowing CLI commands or a different log path.

## Alpha installer

The installer and release build instructions are in `installer.sh` and
`packaging/README.md`. Alpha.5 is the current published release; alpha.6 is a
source tag without published binary assets. Release
binaries target Debian, Ubuntu, and Arch on x86_64.
The installer seeds the root-only monitor config and installs a host-aware
snippet at `/etc/nginx/conf.d/subsite-monitor.conf`, a logrotate policy, and the
systemd unit. The snippet loads when the main Nginx config includes
`conf.d/*.conf` inside `http { }`. The installer checks that include, tests the
Nginx configuration, and reloads `nginx.service` when it is active. If the
configured log is readable, it reloads systemd and enables/starts the monitor
service; on upgrades it restarts an active monitor. It does not edit the main
Nginx config or start an inactive Nginx service. The default monitored log is
`/var/log/subsite-monitor/access.log`.
