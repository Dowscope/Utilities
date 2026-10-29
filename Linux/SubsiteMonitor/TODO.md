# TODO

Planned work, ordered by the project's development phases. Checked items are
complete. Begin each new phase only after the user approves it.

Last reviewed: 2026-10-29. Phases 1 through 5 are implemented in development
version 0.1.0-alpha.8. Alpha.6 is the latest published binary release for
server testing. The alpha.8 archive is built; publication remains pending.
Current default access log:
`/var/log/subsite-monitor/access.log`. Alpha.6 is the latest published binary;
the pushed alpha.7 tag has no GitHub release assets. Alpha.8 includes the
in-UI updater and awaits its final package/release preparation.

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
  installer on Debian/Ubuntu, review ABI/terminfo portability, and publish the
  alpha.8 archive and checksum.
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
  UI is not root, then tells the user to relaunch the UI.
- [ ] **Operational follow-up:** test the installed systemd unit against the
  user's actual Nginx configuration and logrotate policy on the subsites server.
  Confirm installer output shows Nginx validation/reload and monitor service
  activation, then verify log writes and collected requests.
