# Subsite Monitor Project Handoff Prompt

This file preserves the project brief and current handoff state. Update it as
the project changes so a new chat can continue without relying on conversation
history.

## Project brief

Build **Subsite Monitor**, a lightweight, efficient, secure, and portable Linux
CLI application that monitors website activity through existing web-server
access logs. Its primary use is monitoring multiple Nginx virtual hosts on a
Debian server (`subsites-ext`) to identify active and inactive websites. The
project is developed on Arch Linux and is intended for Debian, Ubuntu, and Arch
Linux servers.

Use modern C++23 initially, with room to adopt C++26 as compiler support
matures. Use GCC and GNU Make. Do not introduce CMake, Ninja, Meson, or other
build systems. Prefer the C++ standard library and avoid external dependencies
except ncursesw, which the user later authorized for the terminal UI.
Prioritize execution speed, low memory use, security, RAII, const correctness,
`std::filesystem`, `std::string_view`, small functions, modular files, and
strict compiler warnings. Explicit-log commands must work without root when
the user can read the configured logs. The default UI uses a root-only config
and therefore requires root. The user later authorized an ncurses terminal UI
and a local foreground log-monitoring service managed by systemd. Email,
network-facing services, and databases remain out of scope.

The project is part of the existing `Utilities` Git repository at
`Linux/SubsiteMonitor`. Never initialize another repository or modify files
outside this project. Commit and push only on the user's explicit instruction;
the user has specified that the commit command authorizes updating Markdown,
committing, and pushing to the configured GitHub origin. Do not publish a
release or deploy without approval.
Do not automatically modify Nginx configuration or install development tools
on production servers. Never invent release URLs or assume an Arch binary is
compatible with Debian.

## Development phases

Implement one phase at a time. At the end of each phase, build and run the
appropriate tests, report results, and wait for the user's explicit approval
before starting the next phase.

1. **Project foundation:** inspect existing files and Git status; establish a
   minimal C++23 structure and Makefile; implement basic CLI handling,
   `--help`, and `--version`; compile and test.
2. **Nginx log parser:** document a supported host-aware log format; extract
   timestamp, host, method, URL/path, client IP, and status; safely skip
   malformed records; stream files; add realistic fixtures and
   `collect --log <path>`.
3. **Basic statistics:** aggregate requests and daily counts by website, track
   latest access and malformed lines, and display a sorted aligned table via
   `stats`; test aggregation and reporting.
4. **CSV storage:** write daily CSV statistics with configurable storage,
   correct escaping, safe replacement where supported, concurrent-write
   protection, and `stats --days 7`; test persistence and reporting.
5. **Incremental collection:** persist file identity and offsets; avoid double
   counting; handle rotation, truncation, incomplete lines, concurrent
   collectors, interruption, and checkpoint recovery. Prioritize avoiding
   lost or duplicated data. If crash-safe CSV/checkpoint coordination is too
  complex, document a minimal safe approach before implementing it. A
  foreground `service` command may run under the supplied systemd unit; never
  enable it automatically or change Nginx configuration.
6. **Configuration:** add `config/monitor.conf.example` for access logs,
   statistics directory, and state directory; CLI options override applicable
   values; validate configuration and test it. Store no secrets.
7. **Documentation and release preparation:** document build/test, Nginx
   format, configuration, storage, and limitations; review `install.sh`; plan
   release strategy and Debian/Arch binary compatibility; run a clean final
   build and full suite. Do not publish or deploy without approval.

## CLI target

```text
subsite-monitor --help
subsite-monitor --version
subsite-monitor collect --log /path/to/access.log
subsite-monitor stats
subsite-monitor stats --days 7
subsite-monitor service
```

Use standard-library argument handling, meaningful errors, and appropriate
exit codes. The Makefile should support `make`, `make clean`, `make test`,
`make debug`, and `make install`; use C++23, GCC, `-O2` release builds,
`-Wall -Wextra -Wpedantic -Wconversion`, automatic header dependencies,
parallel builds, conventional install paths, and `DESTDIR`.

## Security and deployment requirements

Treat access logs as untrusted input. Validate lengths and malformed records,
bound memory use, never execute commands using log contents, handle filesystem
errors safely, use restrictive file permissions, and avoid exposing sensitive
URL parameters in reports. Trusted reverse-proxy requirements must be
documented for accurate client IPs. Monitoring failures must not affect the
web server. A local alpha.5 installer/package now prepares the executable,
root-only monitor config, private state/log directories, Nginx `conf.d` snippet,
logrotate policy, and systemd unit. It accepts a checksum-verified
local archive for pre-release testing. It deliberately does not edit the main
Nginx config; it validates the config and reloads `nginx.service` when active.
It does not start an inactive Nginx service. When the configured log is
readable, it enables/starts the monitor service and restarts it on upgrade.
Alpha.5 was published
by the user for server testing; no production deployment is implied.

## Current handoff state

- Current version is `0.1.0-alpha.5`, published for server testing. The user
  asked to add a continuously running CSV-updating service and systemd unit;
  the unit must not be enabled on this development host or a production host
  as part of implementation.
- Phases 1 through 5 are implemented locally: C++23 CLI, bounded streaming
  Nginx parser, per-site/daily aggregation, Docker-backed development, root-only
  log configuration and virtual-host inventory, private daily summary CSVs, and
  per-request CSV event history, checkpointed live collection, and a systemd
  unit installed by `make install`.
- `collect --log <path> [--stats-dir <path>]` scans the full log and replaces
  snapshots for dates present in the scan. Repeating collection on the same
  complete log does not double count. `stats [--days N] [--stats-dir <path>]`
  reads stored data; N selects the newest N stored dates. `stats --log <path>`
  continues to provide a one-time report without persistence.
- Default storage is `/var/lib/subsite-monitor/stats` as root and under the
  user's XDG state directory otherwise. `SUBSITE_MONITOR_STATS_DIR` and
  `--stats-dir` override the location. The directory is private (0700), CSV and
  lock files are private (0600), writers/readers use an advisory lock, and CSV
  snapshots are replaced atomically. Per-request files preserve UTC timestamp,
  site, client IP, method, full URL including query string, and HTTP status.
  Event timestamps are stored in UTC and rendered in the monitor host's local
  timezone with numeric offset.
- In the UI, arrows or `j`/`k` select a site and Enter opens that site's
  request records from CSV in a one-line-per-request table showing timestamp,
  client IP, method, status, and full URL. Up/down selects a request, Page Up/Down moves in 100-record
  pages, left/right scrolls a long URL, Escape returns to the overview, `r`
  refreshes, and `q` exits. The UI collects/stores on launch and refresh. If the log is unavailable, it
  can display stored statistics alongside discovered configured sites.
- Request CSVs contain sensitive client IPs and full URLs. Files are mode 0600
  in a mode 0700 directory; storage grows with request volume.
- `service` follows the configured log and updates event and summary CSVs about
  once per second. It starts at the current log end when no checkpoint exists;
  use a one-time `collect` first for historical backfill. It supports retained
  uncompressed numeric rotations. Copytruncate has a known truncate-and-regrow
  race between polls. Manual full collection is refused while the service is
  active; the UI reads stored data and refreshes on `r`.
- The release installer now reloads systemd and enables/starts the monitor
  service when the configured access log is readable. It restarts the service
  on upgrades and skips activation if systemd or the log is unavailable.
  Unlike manual `make install`, the release installer does not backfill old
  log history before starting the service.
- `./verify.sh` and `./packaging/build-alpha.sh` passed with
  parser, statistics, config, Nginx config, storage, CLI, service, and
  installer-template tests. The C++23 build had no warnings. The local rebuilt
  alpha.5 package (with refreshed documentation) has SHA-256
  `9e61dcdb3dc952388c7cada441062e7ff1e6f949a1e0aec3954a9d1a4aef566d`; the
  archive includes the systemd unit, Nginx snippet, and logrotate policy. The
  already published alpha.5 archive remains SHA-256
  `c2ce6821abe4b95afce815e60c9afb59c1606fdf3812783ff5baf5541ee0235c`.
  `installer.sh` and `install.sh` are rendered with the new Nginx validation
  and reload behavior while pinned to that existing release asset. The
  published alpha.5 archive was downloaded and checksum-verified against the
  release's `SHA256SUMS`. The installer uses a distinct `APP_VERSION`
  variable so Debian's `VERSION` from `/etc/os-release` cannot interfere
  with executable version validation.
- `./run-dev.sh` with no arguments builds and opens the UI, preferring the
  Docker-generated Nginx log/config when present and otherwise using fixtures.
  Start the fixture in another terminal with `sudo ./run-docker.sh`.
- Documentation is in `Docs/MANUAL.md` and `Docs/TECHNICAL.md`; `TODO.md` tracks
  pending phases. When the user says `commit`, update all Markdown, commit, and
  push to the configured GitHub origin. Do not release without approval.

The server-test integration and installer changes are maintained in this
repository. The release installer validates Nginx and reloads its active
systemd service. No server installation has been performed by the developer.

The staged `make install` and `systemd-analyze verify` checks passed. The
release installer places the access log at
`/var/log/subsite-monitor/access.log` by default and installs its snippet in
`/etc/nginx/conf.d`. Nginx loads the snippet when the main config includes
`conf.d/*.conf` inside `http { }`. When Nginx is installed, the installer runs
`nginx -t`, confirms the snippet appears in `nginx -T`, then reloads
`nginx.service` if active. It does not edit the main config or start an
inactive Nginx service. Use the local archive option for pre-release server
testing. Phase 6 remains future work. Validate the automatically started
service and confirm it records requests on the subsites server; do not publish
another release without explicit approval.
