# Changelog

### Installer enables and starts monitor service — 2026-10-29T09:17:34-04:00

- After installing files and validating/reloading Nginx, the release installer
  reloads systemd and enables/starts the monitor when its configured log is
  readable. On upgrades it restarts an active monitor. It skips automatic
  activation when systemd or the log is unavailable, and never starts Nginx.
- Added integration coverage for service enable/start and active-service
  restart behavior. Updated the README, user/technical manuals, packaging guide,
  handoff prompt, and TODO.
- The alpha.5 release archive is unchanged; the tracked Git installer handles
  activation. `./verify.sh` passed.

### Debian installer version variable collision — 2026-10-29T08:46:29-04:00

- Fixed a Debian installer failure where sourcing `/etc/os-release` overwrote
  the installer's `VERSION` variable with the distribution version. The
  application version now uses `APP_VERSION`.
- Added an installer integration test that supplies Debian's
  `VERSION="12 (bookworm)"` field and verifies the alpha.5 binary version
  check still succeeds. `./verify.sh` passed, and the regenerated Git
  installer passed the integration test against the downloaded published
  alpha.5 archive.

## 0.1.0 — 2026-10-27

- Established a C++23/GCC CLI foundation with `--help` and `--version`.
- Added GNU Make build, clean, debug, install, and smoke-test targets.
- Added `run-dev.sh`, `verify.sh`, project handoff prompt, TODO list, and
  technical/user documentation.

### Nginx log path and server-test documentation — 2026-10-28T15:59:22-04:00

- Confirmed the monitored log is kept separately at
  `/var/log/subsite-monitor/access.log`; the installer installs its Nginx
  `conf.d` snippet and logrotate policy. Nginx loads the snippet when its main
  config includes `conf.d/*.conf` in `http { }`.
- Updated the README, user manual, technical reference, packaging guide,
  handoff prompt, and TODO with the installer behavior, test commands, current
  alpha.5 archive checksum, and outstanding server validation.
- Rebuilt the local alpha.5 archive/installers and ran `./verify.sh`; build,
  tests, help, version, and installer-template tests passed without compiler
  warnings. Updated archive SHA-256:
  `c2ce6821abe4b95afce815e60c9afb59c1606fdf3812783ff5baf5541ee0235c`. No
  server deployment or GitHub release was performed.

### Installer validates and reloads Nginx — 2026-10-28T16:27:09-04:00

- Updated the release installer to run `nginx -t`, confirm the Subsite Monitor
  snippet appears in `nginx -T`, then reload `nginx.service` when active. It
  leaves an inactive service stopped and reports when Nginx is unavailable.
- Added installer integration coverage for active and inactive Nginx service
  states, and refreshed the user, technical, packaging, and handoff docs.
- `./verify.sh` and the alpha.5 package build passed with no compiler warnings.
  The locally rebuilt archive with updated documentation has SHA-256
  `9e61dcdb3dc952388c7cada441062e7ff1e6f949a1e0aec3954a9d1a4aef566d`. The
  existing published archive was downloaded and verified at SHA-256
  `c2ce6821abe4b95afce815e60c9afb59c1606fdf3812783ff5baf5541ee0235c`; the Git
  installer scripts are pinned to that existing release asset, so it does not
  need replacement for the installer change. The refreshed Git installer is
  ready for server testing once these changes are pushed.

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


### Phase 4 — CSV storage and site history — 2026-10-27T16:25:46-04:00

- Added private daily CSV snapshots with atomic replacement, restrictive file
  modes, and advisory locking. Collection rescans logs and replaces affected
  dates, so repeat scans do not double count.
- Added `stats` over stored data, `--days`, and configurable storage via
  `--stats-dir` and `SUBSITE_MONITOR_STATS_DIR`.
- The UI now opens the selected site's CSV history with Enter; Escape returns
  to the overview. Updated the user manual, technical reference, README, TODO,
  and handoff prompt.
- `./verify.sh` passed: clean build and all parser, statistics, config, Nginx
  config, storage, CLI, and UI-history tests; no compiler warnings.
- Collection remains a full-log snapshot. Checkpointing and safe recovery from
  log rotation/truncation are Phase 5 work.


### Request-level event history — 2026-10-28T09:26:15-04:00

- Added a private daily event CSV containing every valid request's UTC timestamp,
  site, client IP, method, full URL including query string, and HTTP status.
- Changed Enter on a site to open its request list. Added bounded 100-record
  paging and horizontal scrolling for full URLs. Summary statistics continue to
  use separate daily aggregate CSVs.
- Added storage, CSV escaping, full-query preservation, paging, and pseudo-terminal
  UI checks. `./verify.sh` passed without compiler warnings.
- Event CSVs grow with access-log volume and contain sensitive URLs/IPs. They use
  mode 0600 in the mode 0700 statistics directory. Rotation-safe incremental
  collection remains future work.


### Request table layout — 2026-10-28T10:12:56-04:00

- Changed the request-history screen to a compact table with one request per
  line, showing UTC time, client IP, method/status, and URL. Long selected URLs
  remain horizontally scrollable; paging still covers the full request list.
- `./verify.sh` passed after the UI change.

### Server-local time display — 2026-10-28T10:20:27-04:00

- Changed request-history and last-access displays to the machine's configured
  local timezone, including a numeric UTC offset. Stored timestamps remain UTC.
- Verified `America/Thunder_Bay` renders fixture timestamps with the expected
  `-0400` offset. `./verify.sh` passed without compiler warnings.


### Continuous CSV collection service — 2026-10-28T10:47:24-04:00

- Added `subsite-monitor service`, a foreground log follower that appends
  per-request CSV rows and updates daily summary CSVs about once per second.
- Added checkpoints for log device/inode/offset and committed event-file sizes,
  exclusive service locking, incomplete-line handling, restart recovery,
  truncation detection, and retained numeric logrotate file catch-up.
- Added a systemd unit. `make install` and the release packaging template install
  it and create a private state root, but do not enable or start it. The
  installer/package pipeline now includes the unit for a future approved
  release.
- The UI reads stored CSV data while service collection is active. Manual full
  collection is rejected while the service holds its lock.
- Documented one-time historical backfill, service management, and the
  copytruncate and compressed-rotation limitations.
- Added a service integration test for incomplete lines, live append, rotation,
  restart recovery, truncation, and concurrent collection refusal.
- `./verify.sh` passed, including service integration tests, with no compiler
  warnings. Staged install paths and the systemd unit were also checked.


### Release package includes service unit — 2026-10-28T10:59:13-04:00

- Added the service unit to the release archive and updated the generated
  installer to install it, create the private state directory, and leave the
  service disabled. A custom binary install path is reflected in the unit.
- Added an installer-template integration test to the regular `make test`
  suite.
- Corrected static ncurses terminfo search paths for standard Linux locations;
  the package build now passes the pseudo-terminal UI tests.
- Built the local alpha.5 archive and generated installer scripts. The archive
  was later rebuilt with the Nginx snippet noted below. No GitHub release was
  created.


### Server test installer and Nginx integration — 2026-10-28T14:18:43-04:00

- Moved the monitored log to `/var/log/subsite-monitor/access.log`; the
  installer creates its private directory and migrates the previous built-in
  Nginx log path while preserving custom paths.
- The installer places the host-aware snippet at
  `/etc/nginx/conf.d/subsite-monitor.conf` and adds a daily logrotate policy
  with delayed compression and an Nginx log-reopen signal. It does not edit the
  main Nginx config, reload Nginx, or enable the monitor service. Existing
  snippets/policies are preserved and checked against the configured path.
- Docker development now uses the same log directory and loads the snippet
  through its `conf.d` include.
- `SUBSITE_MONITOR_ARCHIVE` supports checksum-verified local installation
  before GitHub publication. Installer tests cover local archives, path
  migration, custom paths, and preserving existing config files.
- Documented automatic `conf.d` loading when the main config includes it from
  `http { }`, along with manual validation and reload steps.
- Rebuilt the local alpha.5 archive and installer scripts. Current SHA-256:
  `395ea83f7a049503754456214dd8989f23b82d0708bcaccff5d9b328c67bf26a`.
