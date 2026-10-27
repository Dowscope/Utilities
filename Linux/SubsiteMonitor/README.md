# Subsite Monitor

A small C++ command-line utility for monitoring website activity from web-server
access logs. The current Phase 1 foundation provides `--help` and `--version`;
log collection and statistics will be added in later phases.

## Build and test

Requirements: GCC with C++23 support and GNU Make.

```sh
make -j
make test
```

Use `make clean` to remove build outputs, `make debug` for a debug build, and
`make install` to install under `/usr/local` (override `PREFIX` or use
`DESTDIR` for packaging).

For local development, `./run-dev.sh` builds and runs the CLI (defaults to
`--help`). Run `./verify.sh` for a clean build, tests, and CLI smoke checks.
The [user and technical manuals](Docs/README.md) describe current behavior and
design. [Prompt.md](Prompt.md) preserves the project handoff brief,
[Changelog.md](Changelog.md) records changes, and [TODO.md](TODO.md) tracks
approved future work.

The verification script passed on 2026-10-27: it confirmed a clean C++23
build, passing CLI tests, and working help/version output without compiler
warnings. The manuals describe current behavior and limitations.

## Current CLI

```text
subsite-monitor --help
subsite-monitor --version
```
