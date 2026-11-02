# TODO

Planned work, ordered by the project's development phases. Checked items are
complete. Begin each new phase only after the user approves it.

Last reviewed: 2026-11-02. Phases 1 through 5 are implemented in development
version 0.1.0-alpha.15. Alpha.13 is the latest published binary release;
alpha.15's per-site request-period archive is built and checksum-verified but
not yet published.
Current default access log:
`/var/log/subsite-monitor/access.log`. Alpha.9 polishes the UI command bars and
shows update availability only when a compatible release exists. The updater
release ordering fix is included in alpha.12; automatic UI restart after update
is included in alpha.13.

- [x] **Phase 2 — Nginx log parser:** define and document a host-aware log
  format; parse required request fields safely; stream logs; add sample logs,
  parser tests, and `collect --log <path>`.
- [x] **Phase 3 — Basic statistics:** aggregate per-site and daily requests,
  latest access, malformed-line counts, sorted terminal report, and tests.
- [x] **Interactive UI and Nginx inventory:** no-argument UI, root-only log
  config, configured `server_name` discovery, selection, refresh, and request history.
- [x] **Phase 4 — CSV storage:** private daily CSV snapshots, atomic replacement,
  advisory locking, `stats --days`, daily per-request CSVs, full URL/client-IP
  history in the UI, bounded pagination, and tests.
- [x] **Phase 5 — Incremental collection:** checkpoints, offsets, rotation and
  truncation recovery, incomplete lines, concurrent-run prevention, and
  interrupted-write recovery; foreground service and systemd unit added. Numeric
  uncompressed rotations are supported; copytruncate retains a race limitation.
- [ ] **Phase 6 — Configuration:** add statistics/state directories to the
  root-only config and support validated config values with CLI overrides.
- [ ] **Phase 7 — Release preparation:** validate the static binary and
  installer on Debian/Ubuntu, review ABI/terminfo portability, and upload the
  alpha.14 and alpha.15 archives and checksums to their GitHub Releases.
- [x] **Docker-backed development:** Compose Nginx writes host-aware logs to an
  ignored bind mount; `run-dev.sh` uses those files when available.
- [x] **Prepare alpha.5 package:** build and verify the static archive, checksum,
  and installer scripts with the systemd unit, Nginx `conf.d` snippet, and
  logrotate policy. The default log is `/var/log/subsite-monitor/access.log`.
  The published alpha.5 archive is checksum-verified; the local documentation
  rebuild has a distinct checksum and has not replaced the published asset.
- [x] **UI table readability:** remove dim styling from alternating website and
  request-history rows; keep the selected row highlighted in white on blue.
- [x] **Sortable UI tables:** show sites by most recent access by default and
  allow sorting the site and request tables by their displayed columns.
- [x] **In-UI software update:** key `u` checks compatible GitHub releases and
  displays release notes. Installation requires a second explicit `y`
  confirmation and runs the installed release installer, which verifies the
  release archive SHA-256. It uses `sudo` only after that confirmation when the
  UI is not root, then automatically restarts the UI into the newly installed version.
- [x] **UI/service health bar:** show that the UI is running and whether the
  monitor service owns its lock, refreshing the service state every two seconds.
- [x] **Per-site request periods:** default the site table to today's counts;
  press `p` to cycle through 1Y, 6M, 90D, 30D, 7D, and rolling 24H. Press `r`
  to refresh, and sort request counts by the selected period.
- [ ] **Operational follow-up:** test the installed systemd unit against the
  user's actual Nginx configuration and logrotate policy on the subsites server.
  Confirm installer output shows Nginx validation/reload and monitor service
  activation, then verify log writes and collected requests.
