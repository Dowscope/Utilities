# Subsite Monitor User Manual

## What the program does

Subsite Monitor is intended to help identify which hosted websites receive
requests by reading existing web-server access logs. The current Phase 1
version is a command-line shell only: it prints help and version information.
It does not yet read logs or report activity.

## Requirements

- Linux or another environment with GNU Make and GCC.
- A GCC version with C++23 support.
- No external libraries are needed.

## Build and test

From the project directory:

```sh
make -j
make test
```

`make clean` removes the executable and generated object/dependency files.
`make debug` builds with debug symbols and no optimization. `make install`
installs the executable under `/usr/local/bin` by default. Set `PREFIX` to
change the prefix, or set `DESTDIR` to stage installation for packaging.

The helper scripts provide common local workflows:

- `./run-dev.sh` builds as needed and runs the program. With no arguments it
  shows help; pass CLI arguments to run a specific command.
- `./verify.sh` performs a clean build, runs tests, and checks help and version
  output. It passed on 2026-10-27 with no compiler warnings.

## Commands currently available

```sh
subsite-monitor --help
subsite-monitor --version
```

Running without arguments prints help. Unsupported or incomplete arguments
produce an error on standard error and exit with status 2. Help and version
exit with status 0.

## Planned behavior

Log collection, activity statistics, CSV history, incremental checkpoints, and
configuration are planned in later phases. They are not available in this
version. See the project [TODO](../TODO.md) for the phase list. The required
Nginx log format and trusted reverse-proxy guidance will be documented when
the parser is implemented.

Last reviewed: 2026-10-27. This manual describes Phase 1 only.
