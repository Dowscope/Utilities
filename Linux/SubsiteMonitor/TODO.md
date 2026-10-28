# TODO

Planned work, ordered by the project's development phases. Checked items are
complete. Begin each new phase only after the user approves it.

Last reviewed: 2026-10-28. Phases 1 through 5 are implemented locally in
0.1.0-alpha.5. Alpha.3 remains the latest published version. The alpha.5
archive and generated installers are prepared locally; this does not publish a
release. Current default access log: `/var/log/subsite-monitor/access.log`.

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
  installer on Debian/Ubuntu, review ABI/terminfo portability, then decide
  release contents and notes. Documentation and local clean-build/test work are
  current; do not publish until explicitly requested.
- [x] **Docker-backed development:** Compose Nginx writes host-aware logs to an
  ignored bind mount; `run-dev.sh` uses those files when available.
- [x] **Prepare alpha.5 package:** build and verify the static archive, checksum,
  and installer scripts with the systemd unit, Nginx `conf.d` snippet, and
  logrotate policy. The default log is `/var/log/subsite-monitor/access.log`.
  Local archive installation is checksum-verified. The release itself is not
  published.
- [ ] **Operational follow-up:** test the installed systemd unit against the
  user's actual Nginx configuration and logrotate policy on the subsites server.
  Confirm `nginx -T` shows the `conf.d` snippet under `http`, test with
  `nginx -t`, then reload Nginx and verify log writes before enabling the
  monitor service.
