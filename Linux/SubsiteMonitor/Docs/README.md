# Subsite Monitor Documentation

This directory contains the user manual and technical reference. The current
implementation covers Phases 1 through 5, the ncurses UI, root-only access-log
configuration, Nginx virtual-host discovery, Docker-backed development, daily
summary CSVs, paginated per-request event CSV history, and a checkpointed
foreground service with a systemd unit. Phase 6 configuration expansion remains
future work.

## Contents

- [User manual](MANUAL.md): build, run, CLI behavior, request-history controls,
  storage, and current limitations.
- [Technical reference](TECHNICAL.md): source layout, build details, exit
  behavior, CSV schema, service checkpoint design, and security constraints.
- Docker Compose runs a local Nginx process whose config and log are shared
  with the host development app; see the user manual.

The manuals cover no-argument UI startup, root-only configuration and
permissions, Nginx `server_name` discovery, the packaged host-aware log snippet
and its automatic loading when `conf.d/*.conf` is included inside `http { }`,
local archive installation, and request detail privacy and storage behavior.

The current development version is alpha.6; its UI uses normal brightness for
unselected rows and bold white on blue for the selected row. Alpha.5 remains
the latest published binary release.

Last reviewed: 2026-10-29.
