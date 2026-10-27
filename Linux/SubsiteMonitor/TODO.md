# TODO

Planned work, ordered by the project's approved development phases. These
items are not implemented yet; begin a new phase only after the user approves
it.

Last reviewed: 2026-10-27. Phase 1 verification passes; Phase 2 has not been
approved or started.

- [ ] **Phase 2 — Nginx log parser:** define and document a host-aware log
  format; parse required request fields safely; stream logs; add sample logs,
  parser tests, and `collect --log <path>`.
- [ ] **Phase 3 — Basic statistics:** aggregate per-site and daily requests,
  latest access, and malformed-line counts; add a sorted terminal table,
  `stats`, and tests.
- [ ] **Phase 4 — CSV storage:** persist daily statistics with configurable
  directory, escaping, safe replacement, concurrency handling, and
  `stats --days`; test storage and reporting.
- [ ] **Phase 5 — Incremental collection:** checkpoints, offsets, rotation and
  truncation recovery, incomplete lines, concurrent-run prevention, and
  interrupted-write recovery; test repeated collection and recovery.
- [ ] **Phase 6 — Configuration:** add an example config for log paths,
  statistics directory, and state directory; validate settings and CLI
  overrides.
- [ ] **Phase 7 — Release preparation:** complete documentation, review the
  installer, document release and ABI compatibility strategy, and run a clean
  final build and test suite. Do not publish or deploy without approval.
