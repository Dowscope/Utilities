# Changelog

### Alpha.12 release preparation — 2026-10-29T15:11:55-04:00

- Bumped development and packaging version to `0.1.0-alpha.12`; alpha.11 is
  the latest published binary release.
- Sort compatible releases by semantic version, newest first, in the installer
  picker and in-UI updater rather than trusting GitHub API order.
- Added tests for release ordering and selecting the latest version by default.
- Release title: **Subsite Monitor 0.1.0-alpha.12 — Correct Release
  Ordering**. Archive SHA-256:
  `8ba1e0bf110a6d689bea3901a879575182667c660580cd350f6583f65d955ffc`.
- `./verify.sh` and `./packaging/build-alpha.sh` passed without compiler
  warnings; the archive checksum verified successfully.

### Alpha.11 release preparation — 2026-10-29T15:00:03-04:00

- Bumped development and packaging version to `0.1.0-alpha.11`; alpha.10 is
  the latest published binary release.
- Fixed the updater confirmation screen so it waits for a choice instead of
  inheriting the overview's 250 ms input timeout.
- Release title: **Subsite Monitor 0.1.0-alpha.11 — Reliable Update
  Confirmation**. Archive SHA-256:
  `c36fbbd197158501a38c97348880ad1908d51624c19092a430109ecb9f045cf5`.
- `./verify.sh` and `./packaging/build-alpha.sh` passed without compiler
  warnings; the archive checksum verified successfully.

### Keep updater confirmation open — 2026-10-29T14:42:22-04:00

- Fixed the update confirmation screen closing after 250 ms. It now waits for
  the user's choice, while the overview retains its background refresh timeout.
- Clarified the confirmation behavior in the user and technical manuals.

### Alpha.10 release preparation — 2026-10-29T14:27:55-04:00

- Bumped development and packaging version to `0.1.0-alpha.10`; alpha.9 is the
  latest published binary.
- Added the running version to the overview and request-history headers, and
  the GitHub Issues URL to the overview and CLI help.
- Release title: **Subsite Monitor 0.1.0-alpha.10 — Version and Support
  Details**. Archive SHA-256: `6f23a76d8bbb9aa9c9c449eeab57f1c3576ea304c5c70cd518584620b15c28b0`.
- `./verify.sh` and `./packaging/build-alpha.sh` passed without compiler warnings; archive checksum verification succeeded.

### Display version and issue tracker in the UI — 2026-10-29T14:24:17-04:00

- Added the running version to the site overview and request-history headers.
- Added the GitHub issue-tracker URL to the site overview and CLI help output.
- Added pseudo-terminal and help-output assertions and documented the support
  URL and header labels. This is post-alpha.9 source work.

### Alpha.9 release package prepared — 2026-10-29T14:04:00-04:00

- Bumped development and packaging version to `0.1.0-alpha.9`. Alpha.8 is the
  latest published binary release.
- `./verify.sh` and `./packaging/build-alpha.sh` passed without compiler
  warnings. The archive SHA-256 is
  `43afe61193201337797db932b96d23033f77c83d5f1f2d928d2a4fb83497cd50`.
- Release title: **Subsite Monitor 0.1.0-alpha.9 — Clearer UI Commands and
  Update Notices**.
- Attach the generated archive and `SHA256SUMS` to the alpha.9 GitHub Release.

### Match command-bar action/key labels — 2026-10-29T14:02:44-04:00

- Changed overview, request-history, and update-review footers to the
  `Action: key | Action: key` layout used in the LazyGit reference, retaining
  the existing Subsite Monitor colors.
- Updated pseudo-terminal assertions and documentation.

### Group UI footer commands — 2026-10-29T13:54:00-04:00

- Reorganized the overview, request-history, and update-review command bars
  into task-labeled groups separated by vertical dividers.
- Updated pseudo-terminal checks and manual/technical documentation.

### Show update prompt only when applicable — 2026-10-29T13:51:52-04:00

- Removed the `u Update` hint from the command bar. The lower-right update
  indicator now appears only when a newer compatible release is available.
- Updated the UI tests and manuals to reflect the quieter no-update state.

### Background update availability indicator — 2026-10-29T12:57:17-04:00

- The UI checks for newer compatible releases in the background and shows a
  compact lower-right indicator with the available version and `u` shortcut.
  The release review and confirmation are still required to install an update.
- Added a pseudo-terminal check for the update indicator. The updater replaces
  the packaged executable and service unit, refreshes the installer helper,
  restarts an active monitor service, and reloads active Nginx when applicable;
  the open UI must be relaunched to use the updated executable.

### Remove numeric prefixes from UI column headings — 2026-10-29T12:52:01-04:00

- Removed sort-key numbers from the site and request-history column headings.
  Keyboard shortcuts remain listed in the view footers.
- Updated the user and technical manuals. Existing test expectations now match
  the plain column labels.

### In-UI verified software updater — 2026-10-29T12:27:40-04:00

- Added `u` in the site overview to fetch up to ten newer releases with a
  compatible x86_64 archive and published SHA-256 digest. The UI displays
  release titles, dates, and notes; user-controlled display text is sanitized.
- Enter opens a separate confirmation screen. Only `y` invokes the installed
  release installer, which downloads the selected version and verifies its
  archive digest before replacing the executable. It uses `sudo` when the UI
  is not root and reports that the UI must be relaunched after success.
- The release installer installs and refreshes its helper at
  `/usr/share/subsite-monitor/installer.sh`; `make install` installs the same
  helper. The installer may reload Nginx and restart the monitoring service.
- Added release parser/version comparison unit tests, a pty test for the update
  and confirmation screens, and installer tests for the installed helper.
  `./verify.sh` and `./packaging/build-alpha.sh` passed without compiler
  warnings. The alpha.8 archive SHA-256 is
  `f565f158c1fe8fb21ce0084f74e8c8aca4f7a15b0b1a873437575115c490fe8c`.

### Sortable site overview and alpha.7 package — 2026-10-29T10:42:06-04:00

- The site overview now opens with most recently active sites first and sorts
  by website, request count, or last access using keys `1`–`3`; repeating a key
  reverses the direction. The selected row resets to the top after changing
  columns.
- Added tests for the new overview controls and kept newest-first request
  history sorting within its current 100-record page.
- Added the proposed in-UI software update flow to `TODO.md`; it is not part of
  this alpha.7 release.
- Bumped development and packaging version to `0.1.0-alpha.7`. `./verify.sh`
  and `./packaging/build-alpha.sh` passed with no compiler warnings. Alpha.6 is
  the latest published binary. The alpha.7 archive SHA-256 is
  `b824aa354ac7b7ff7e7940f5d077791cfd7f4ceeda0b148ad646f518fc405d23`.

### Newest-first request history and page sorting — 2026-10-29T10:11:29-04:00

- The detailed site view opens on the latest 100 requests, with the newest at
  the top. Page navigation follows chronological order toward older/newer data.
- Added sorting by timestamp, client IP, method/status, and URL using keys 1–4;
  repeat a key to toggle ascending/descending. Sorting is limited to the loaded
  page so memory stays bounded.
- `./verify.sh` passed.

### Release selection and alpha.6 package — 2026-10-29T09:49:30-04:00

- The installer now offers up to 10 recent releases with compatible x86_64
  archives and valid SHA-256 metadata. Interactive installs show a numbered
  menu; non-interactive installs choose the newest. Added `--latest`, `--list`,
  and `--version <version>` options. Remote installs require Python 3 plus
  curl or wget.
- Added installer tests for API parsing, filtering releases without compatible
  assets, and automatic latest selection. `./verify.sh` and
  `./packaging/build-alpha.sh` passed.
- Built `packaging/dist/subsite-monitor-0.1.0-alpha.6-linux-x86_64-gnu.tar.gz`.
  Its SHA-256 is
  `cdd784ae4d029ff370327260f8b735382e0d596d19c14be3719f6e52ae061903`.
  GitHub release creation and asset upload remain pending; this environment
  has no authenticated GitHub release-management tool.

### Improve table readability — 2026-10-29T09:28:48-04:00

- Removed dim styling from alternating website and request-history rows. All
  unselected rows now use the terminal's normal text color; the selected row
  remains bold white on blue.
- Bumped the development version to `0.1.0-alpha.6` and tagged the source as
  `v0.1.0-alpha.6`. No GitHub release or binary assets were published; the
  existing alpha.5 installer and release archive remain unchanged.
- `./verify.sh` passed with no compiler warnings.

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
