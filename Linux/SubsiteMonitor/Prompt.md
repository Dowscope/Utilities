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
build systems. Prefer the C++ standard library and avoid external dependencies.
Prioritize execution speed, low memory use, security, RAII, const correctness,
`std::filesystem`, `std::string_view`, small functions, modular files, and
strict compiler warnings. The app must work without root when the user can read
the configured logs. v0.1.0 has no GUI, ncurses, email, network service, or
database.

The project is part of the existing `Utilities` Git repository at
`Linux/SubsiteMonitor`. Never initialize another repository, modify files
outside this project, commit without the user's commit command, push without
the user's push command, or publish a release or deploy without approval.
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

- Completed Phase 1 only: minimal C++23 CLI, `--help`, `--version`, Makefile,
  smoke test, and brief README.
- Phase 1 was built with `make -j`; `make test` passed; help/version output was
  checked. The latest `./verify.sh` run passed clean build, tests, help, and
  version checks with no warnings. `run-dev.sh --version` also passed.
- Added `run-dev.sh`, `verify.sh`, `Changelog.md`, `TODO.md`, and user/technical
  manuals under `Docs/`.
- No parser, collector, statistics, persistence, or configuration behavior is
  implemented yet.
- Next implementation phase is Phase 2, and it requires the user's explicit
  approval before work begins.
- `run-dev.sh` is for building and running the local CLI. `verify.sh` is the
  repeatable local build and test check.
- When the user gives the commit command, update all Markdown documentation
  files to reflect the current project state, then commit and push to the
  configured GitHub origin as requested. No commit or push is part of normal
  phase implementation work.
