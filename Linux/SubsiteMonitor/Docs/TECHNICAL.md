# Subsite Monitor Technical Reference

## Current architecture

The Phase 1 executable has one translation unit, `src/main.cpp`. It contains
the entry point, a private version constant (`0.1.0`), and a small help
printer. It inspects the first argument as a `std::string_view`; no dynamic
argument framework or third-party dependency is used.

The existing `collector.*` and `report.*` files are empty placeholders and are
not part of the Phase 1 build. Future phases should add implementation only as
each feature is approved, keeping parsing, collection, and reporting separate.

## Build details

The Makefile uses `g++`, `-std=c++23`, `-O2`, and the warning flags
`-Wall -Wextra -Wpedantic -Wconversion`. Compilation emits `.d` dependency
files through `-MMD -MP`, so header changes trigger rebuilds. Parallel jobs are
enabled by invoking GNU Make with `-j` (for example `make -j`).

The default install prefix is `/usr/local`, with the executable installed in
`bin`. `DESTDIR` prepends a staging root to the install destination. No install
step is needed to run the binary from the project directory.

## CLI and process behavior

- `--help` and `-h`: print usage and return 0.
- `--version` and `-V`: print `subsite-monitor 0.1.0` and return 0.
- No arguments: print help and return 0.
- Any other argument shape: print an error and a help hint to standard error;
  return 2.

The current program does not access the filesystem, network, or system
configuration.

## Verification

`tests/test_cli.sh` is a dependency-free shell smoke test. `make test` invokes
it with the built executable. `verify.sh` runs `make clean`, builds in parallel,
runs tests, and checks the help and version commands. The 2026-10-27 run passed
all checks without compiler warnings.

## Security and operational constraints

The Phase 1 CLI has no log input yet. When parsing is added, log contents must
be treated as untrusted input, parsing must use bounded memory, and file errors
must not affect the web server. Reports must avoid leaking sensitive URL query
parameters. Client IP attribution through a reverse proxy will require trusted
proxy configuration. The application should not require root when filesystem
permissions allow access to logs and its own state directories.

Checkpoint durability, CSV write safety, lock behavior, supported log syntax,
and configuration semantics are not defined by the current implementation.
Document those specifics alongside the phase that introduces them.

Last reviewed: 2026-10-27. This reference describes Phase 1 only.
