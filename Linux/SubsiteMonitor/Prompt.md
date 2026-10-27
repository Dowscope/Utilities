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
strict compiler warnings. The app must work without root when the user can read
the configured logs. The initial brief excluded GUI/ncurses, email, network
service, and database. The user later authorized an ncurses terminal UI;
email, network service, and database remain out of scope.

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
   complex, document a minimal safe approach before implementing it.
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
web server. Eventually an installer may detect platform/architecture, fetch a
real release artifact, verify trusted checksums/signatures, install to
`/usr/local/bin/subsite-monitor`, create `/etc/subsite-monitor/` and
`/var/lib/subsite-monitor/`, preserve existing config/history, and support
upgrades. This is future planning only; do not implement production deployment
without explicit authorization.

## Current handoff state

- Handoff refreshed 2026-10-27T13:57:07-04:00 for the alpha installer follow-up.
- Completed Phases 1 through 3: C++23 CLI, streaming Nginx parser,
  `collect --log`, per-site/daily statistics via `stats --log`, sample logs,
  parser/statistics/CLI tests, and user/technical documentation.
- `stats` currently requires an explicit log path because persistence and
  configuration have not been implemented. It reports per-site totals, latest
  access in UTC, daily counts by log date, and malformed lines.
- Phase 3 passed `./verify.sh`: clean build, parser/statistics/CLI tests,
  help, and version checks; no compiler warnings.
- Statistics aggregation is capped at 10,000 sites and 100,000 site/date pairs
  to keep memory use bounded. The report never prints request targets or
  client addresses.
- Implemented the requested ncurses UI. `ui --log <path>` displays a one-log
  snapshot; arrows or `j`/`k` select a site, `r` rereads the log, and `q` exits.
  The sample UI was run interactively and selection/refresh/quit were exercised.
  Ncursesw and pkg-config are build dependencies.
- `./verify.sh` passes after the TUI addition, and the sample screen reports 5
  requests across 2 sites with 2 malformed lines.
- `./run-dev.sh` with no arguments builds and opens the sample-log UI; supplied
  arguments are forwarded to the CLI. This was verified in a pseudo-terminal.
- Added `run-dev.sh`, `verify.sh`, `Changelog.md`, `TODO.md`, and user/technical
  manuals under `Docs/`.
- The alpha package work was refreshed locally as of 2026-10-27T13:50:57-04:00.
  `packaging/build-alpha.sh` builds x86_64 Linux release archive
  `subsite-monitor-0.1.0-alpha.1-linux-x86_64-gnu.tar.gz` and generates
  checksum-pinned root scripts `installer.sh` and `install.sh`. The executable
  statically links ncursesw and GNU runtimes, and its UI uses host terminfo.
  The locally generated archive SHA-256 is
  `17c42c282df629e00124eb439a6d0da76b88fec700a793af4afcadef48ad755d`.
- Package build, parser/statistics/CLI tests, and successful/corrupted archive
  installer tests passed. No GitHub release or release assets have been
  published. The release asset must be attached before the installer can
  download it.
- A follow-up fixed generated `SHA256SUMS` to contain a portable relative
  filename and rebuilt the archive after refreshing the README/manual included
  in the package. The build script and root installer use the same archive
  hash. Attach this archive and checksum manifest to the GitHub release before
  installing; the release itself is still pending.
- The installer supports x86_64 Debian, Ubuntu, and Arch, checks the OS and
  archive checksum, and installs only the executable to `/usr/local/bin` by
  default. It does not set up persistent stats/configuration or system
  services. Validate the static glibc artifact on target hosts before
  production use.
- Phase 4 (CSV storage) is the next implementation phase; it requires the
  user's explicit approval before work begins.
- `run-dev.sh` builds and opens the ncurses UI on the bundled sample log when
  run without arguments; supplied arguments are passed to the executable.
  `verify.sh` is the repeatable local build and test check.
- When the user gives the commit command, update all Markdown documentation
  files to reflect the current project state, then commit and push to the
  configured GitHub origin as requested. No commit or push is part of normal
  phase implementation work.
