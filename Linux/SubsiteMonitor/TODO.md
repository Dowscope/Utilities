# TODO

Planned work, ordered by the project's development phases. Checked items are
complete. Begin each new phase only after the user approves it.

Last reviewed: 2026-10-27T13:57:07-04:00. Phases 1 through 3 and the ncurses
screen are implemented. The alpha package is built locally; GitHub publication
is pending. Phase 4 remains the next implementation phase.

- [x] **Phase 2 — Nginx log parser:** define and document a host-aware log
  format; parse required request fields safely; stream logs; add sample logs,
  parser tests, and `collect --log <path>`.
- [x] **Phase 3 — Basic statistics:** aggregate per-site and daily requests,
  latest access, and malformed-line counts; add a sorted terminal table,
  `stats --log`, and tests.
- [x] **Interactive CLI screen:** add an ncurses statistics screen with site
  selection, daily detail, manual refresh, and quit controls.
- [ ] **Phase 4 — CSV storage:** persist daily statistics with configurable
  directory, escaping, safe replacement, concurrency handling, and
  `stats --days`; test storage and reporting.
- [ ] **Phase 5 — Incremental collection:** checkpoints, offsets, rotation and
  truncation recovery, incomplete lines, concurrent-run prevention, and
  interrupted-write recovery; test repeated collection and recovery.
- [ ] **Phase 6 — Configuration:** add an example config for log paths,
  statistics directory, and state directory; validate settings and CLI
  overrides.
- [x] **Phase 7 — Release preparation:** document release and ABI strategy,
  review installer behavior, and build/test an alpha archive locally.
- [ ] **Publish alpha release:** create GitHub release `v0.1.0-alpha.1` and
  attach the generated archive plus `SHA256SUMS`; then
  verify the raw `installer.sh` URL and release asset download.
