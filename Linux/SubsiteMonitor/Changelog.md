# Changelog

## 0.1.0 — 2026-10-27

- Established a C++23/GCC CLI foundation with `--help` and `--version`.
- Added GNU Make build, clean, debug, install, and smoke-test targets.
- Added `run-dev.sh`, `verify.sh`, project handoff prompt, TODO list, and
  technical/user documentation.

### Documentation and verification update — 2026-10-27T11:42:32-04:00

- Refreshed the project handoff, user manual, technical reference, README, and
  TODO to match the current Phase 1 implementation.
- Ran `./verify.sh`: clean build, CLI tests, help, and version checks passed
  without compiler warnings.

### Phase 2 — 2026-10-27T11:49:28-04:00

- Added a bounded streaming parser for the documented host-aware Nginx format
  and `collect --log <path>` with parsed/malformed line counts.
- Added sample access logs, parser and CLI coverage, and Nginx format,
  field-limit, and trusted-proxy documentation.
- `./verify.sh` passed: clean build, tests, help, and version checks completed
  without compiler warnings.

### Phase 3 — 2026-10-27T12:02:24-04:00

- Added `stats --log <path>` with per-site request totals, daily counts,
  malformed-line count, and latest access normalized to UTC.
- Added a sorted terminal report and aggregation/reporting tests; reports omit
  request targets and client addresses.
- Capped aggregation at 10,000 sites and 100,000 site/date pairs to bound
  memory use.
- `./verify.sh` passed without compiler warnings.

### Ncurses CLI screen — 2026-10-27T12:14:15-04:00

- Added `ui --log <path>` with a colored overview, sortable site list,
  per-site daily detail, manual refresh, and keyboard navigation.
- Added ncursesw build requirements and updated the user/technical manuals.
- Ran the UI in a pseudo-terminal and exercised site selection, refresh, and
  quit. `./verify.sh` passed without compiler warnings.

### Development launcher — 2026-10-27T12:16:12-04:00

- Changed `./run-dev.sh` with no arguments to build and open the UI on the
  bundled sample log. Verified the no-argument launch in a pseudo-terminal.

### Commit preparation — 2026-10-27T12:20:44-04:00

- Refreshed all Markdown handoff, user, technical, and tracking documents.
- `./verify.sh` passed: clean build, all tests, help, and version checks.

### Alpha release packaging — 2026-10-27T12:40:16-04:00

- Added a reproducible x86_64 alpha package build and pinned installer
  (`installer.sh`, with `install.sh` retained as a compatibility name).
- Built the application with statically linked ncursesw and GNU runtime
  libraries; the terminal UI still uses the target's terminfo database.
- Added archive checksum verification, platform checks, atomic executable
  replacement, and tests for successful installation and corrupted archives.
- Generated alpha archive:
  `subsite-monitor-0.1.0-alpha.1-linux-x86_64-gnu.tar.gz` (SHA-256:
  `668a8ccc69bfc0c7beb65652ef0a909c2481ad1e11f220bc20239861c07b7d94`).
- `./packaging/build-alpha.sh` passed, including parser/statistics/CLI tests
  and installer checks. GitHub release upload and publication remain pending.
