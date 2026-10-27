# Subsite Monitor Documentation

This directory is the user and technical manual for Subsite Monitor. It will
be kept aligned with implemented behavior as the project grows. The current implementation covers Phases 1 through 3, the ncurses screen,
root-only settings for the access log and Nginx config, and discovery of
configured virtual hosts, including an unavailable-log fallback. CSV persistence and incremental checkpoints remain
planned work.

## Contents

- [User manual](MANUAL.md): build, run, CLI behavior, and current limitations.
- [Technical reference](TECHNICAL.md): source layout, build details, exit
  behavior, and security/design constraints.
- Docker Compose runs a local Nginx process whose config and log are shared
  with the host development app; see the user manual.

The manuals reflect no-argument UI startup, root-only configuration and
permissions, Nginx `server_name` discovery, host-aware log requirements, alpha runtime,
and packaging details.

Last reviewed: 2026-10-27.
