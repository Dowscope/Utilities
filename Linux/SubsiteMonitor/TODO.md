# TODO

Planned work, ordered by the project's development phases. Checked items are
complete. Begin each new phase only after the user approves it.

Last reviewed: 2026-10-27. Phases 1 through 3, the ncurses screen, root-only
config, and Nginx `server_name` inventory are implemented locally. Alpha.2 is
published; alpha.3 is the current development version and has not been built
for release. Phase 4 remains the next planned implementation phase.

- [x] **Phase 2 — Nginx log parser:** define and document a host-aware log
  format; parse required request fields safely; stream logs; add sample logs,
  parser tests, and `collect --log <path>`.
- [x] **Phase 3 — Basic statistics:** aggregate per-site and daily requests,
  latest access, and malformed-line counts; add a sorted terminal table,
  `stats --log`, and tests.
- [x] **Interactive CLI screen:** add an ncurses statistics screen with site
  selection, daily detail, manual refresh, and quit controls.
- [x] **Default UI, root-only config, and Nginx site inventory:** start the UI
  without a command, load configured log and Nginx paths, merge observed hosts
  with literal/wildcard `server_name` entries, and show zero-traffic sites.
- [ ] **Phase 4 — CSV storage:** persist daily statistics with configurable
  directory, escaping, safe replacement, concurrency handling, and
  `stats --days`; test storage and reporting.
- [ ] **Phase 5 — Incremental collection:** checkpoints, offsets, rotation and
  truncation recovery, incomplete lines, concurrent-run prevention, and
  interrupted-write recovery; test repeated collection and recovery.
- [ ] **Phase 6 — Configuration:** extend the implemented access-log setting
  with statistics/state directories and full CLI overrides; validate settings.
- [x] **Phase 7 — Release preparation:** document release and ABI strategy,
  review installer behavior, and build/test an alpha archive locally.
- [x] **Publish alpha 0.1.0-alpha.1:** attach the release archive and checksum,
  then verify installation on the user's development server.
- [x] **Publish alpha 0.1.0-alpha.2:** attach the release archive and
  checksum, then confirm installation on the server.
- [ ] **Prepare alpha 0.1.0-alpha.3:** build and test a release archive with
  Nginx site discovery, then review release notes before publishing.
