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

### Alpha release packaging — 2026-10-27T13:50:57-04:00

- Added a reproducible x86_64 alpha package build and pinned installer
  (`installer.sh`, with `install.sh` retained as a compatibility name).
- Built the application with statically linked ncursesw and GNU runtime
  libraries; the terminal UI still uses the target's terminfo database.
- Added archive checksum verification, platform checks, atomic executable
  replacement, and tests for successful installation and corrupted archives.
- Generated alpha archive:
  `subsite-monitor-0.1.0-alpha.1-linux-x86_64-gnu.tar.gz` (SHA-256:
  `17c42c282df629e00124eb439a6d0da76b88fec700a793af4afcadef48ad755d`).
- `./packaging/build-alpha.sh` passed, including parser/statistics/CLI tests
  and installer checks. GitHub release upload and publication remain pending.
- After checking installer behavior against the unpublished release URL, made
  the generated `SHA256SUMS` path relative so release users can verify it from
  the extracted download directory.

### Installer 404 follow-up — 2026-10-27T13:57:07-04:00

- Regenerated the installer and release archive after refreshing packaged
  documentation, so the embedded checksum matches the current artifact
  (`17c42c282df629e00124eb439a6d0da76b88fec700a793af4afcadef48ad755d`).
- Fixed the packaging script to write a portable relative filename in
  `SHA256SUMS`; the full package build and installer tests passed again.
- The GitHub release remains unpublished; attach the refreshed archive and
  checksum manifest to `v0.1.0-alpha.1` before using the installer.

### Published alpha 0.1.0-alpha.1 — 2026-10-27T13:58:28-04:00

- Published the release archive and verified installer download and
  installation on the user's development server.

### Default UI and root-only configuration — 2026-10-27T14:24:29-04:00

- Changed no-argument startup and bare `ui` to open the ncurses screen from the
  configured shared Nginx access log.
- The UI counts distinct valid hostnames in the log; sites with no valid
  requests cannot be discovered from access logs alone.
- Added the root-owned `/etc/subsite-monitor/monitor.conf` format for the log
  path. The installer seeds it with mode 0600 under a mode 0700 directory and
  preserves existing values on upgrade.
- Config reading checks root ownership and permissions; the config parser
  validates the setting, path, size, and control characters.
- Bumped the development version to `0.1.0-alpha.2`. Clean build, all tests,
  pseudo-terminal UI check (2 fixture websites), staged install checks, and
  static package/installer tests passed.
- Built `subsite-monitor-0.1.0-alpha.2-linux-x86_64-gnu.tar.gz` with SHA-256
  `2d3e312c48c58000e8cfc403ab355d1ca6ec2e5d95b034623e1858f82edb30b3`. The
  alpha.2 release has not been published.

### Commit preparation — 2026-10-27T14:29:16-04:00

- Refreshed project handoff, release notes, TODO, and user/technical manuals.
- Verified archive checksum, installer hash pins, shell syntax, and packaged
  README/manual contents before committing.


### Nginx site inventory — 2026-10-27

- Added bounded scanning of the configured Nginx main config and included files
  to discover literal and wildcard `server_name` entries. Regex names and `_`
  are skipped.
- The default UI merges configured names with log activity, so configured sites
  without requests remain visible with zero activity. Added `nginx_config` to
  the root-only config and `SUBSITE_MONITOR_NGINX_CONFIG` override.
- Added realistic config fixtures and parser coverage for includes, duplicate
  names, wildcard names, and malformed configuration.
- Updated the manual and technical reference. `./verify.sh` passed a clean
  C++23 build and all tests without compiler warnings.
- Built and published `0.1.0-alpha.3`; the user confirmed installing it on the
  target server. Archive SHA-256: `4b0b709a5688d4a18e0febcbf0e324fdb4aac84decd98e18e4d7c3bf5c61ac81`.


### Missing access-log fallback — 2026-10-27

- Fixed the TUI to continue displaying configured Nginx sites when the selected
  access log cannot be opened, with a clear warning that activity is unavailable.
- Corrected `verify.sh` to check the actual version string; it previously checked
  `0.1.0` and would fail after the build/tests.
- Verified the unavailable-log behavior in a pseudo-terminal: configured fixture
  sites appeared alongside the warning. `./verify.sh` passed.
- Development version is now `0.1.0-alpha.4`; packaging and publication remain
  pending.


### Docker-backed development workflow — 2026-10-27

- Compose runs a local Nginx service with three virtual hosts and host-aware
  access logging. `run-dev.sh` builds/runs the app natively and automatically
  reads the shared Docker config/log when present, falling back to fixtures.
- Verified Compose configuration and `./verify.sh`. The host UI read the
  Docker-generated log in a pseudo-terminal and displayed 11 requests across
  `alpha.test`, `beta.test`, and inactive `inactive.test`, with no malformed
  lines.
- Scoped generated `.o`/`.d` ignores to `src/`; the old broad `*.d` pattern
  also hid directories named `conf.d`. The Compose vhost file and Nginx parser
  include fixtures are now tracked, and `./verify.sh` passes with them present.
