# Subsite Monitor

A small C++ command-line utility for monitoring website activity from web-server
access logs. It streams the documented Nginx log format, reports per-site and
daily statistics, and provides an ncurses screen. CSV history and configuration
are planned for later phases.

## Build and test

Requirements: GCC with C++23 support, GNU Make, `pkg-config`, and ncursesw
development files.

```sh
make -j
make test
```

Use `make clean` to remove build outputs, `make debug` for a debug build, and
`make install` to install under `/usr/local` (override `PREFIX` or use
`DESTDIR` for packaging).

For local development, `./run-dev.sh` builds and opens the ncurses UI with the
bundled sample log. Pass arguments to run another command or log. Run
`./verify.sh` for a clean build, tests, and CLI smoke checks.
The [user and technical manuals](Docs/README.md) describe current behavior and
design. [Prompt.md](Prompt.md) preserves the project handoff brief,
[Changelog.md](Changelog.md) records changes, and [TODO.md](TODO.md) tracks
approved future work.

The verification script passed on 2026-10-27T12:20:26-04:00: clean C++23 build,
parser/statistics/CLI tests, help, and version checks. The manuals describe
current behavior and limitations.

## Current CLI

```text
subsite-monitor --help
subsite-monitor --version
subsite-monitor collect --log /path/to/access.log
subsite-monitor stats --log /path/to/access.log
subsite-monitor ui --log /path/to/access.log
```

`collect` prints the number of parsed and malformed lines. It does not print
request targets or client addresses. See the [manual](Docs/MANUAL.md) for the
supported Nginx format, parser limits, and statistics report. The current
`stats` command reads one log directly, with caps of 10,000 distinct websites
and 100,000 website/date pairs; CSV-backed history is a later step.

`ui` opens an interactive ncurses screen for the same statistics. Use the
arrow keys or `j`/`k` to select a site, `r` to reread the log, and `q` to quit.
It requires a real interactive terminal. For the current sample data, run:

```sh
./run-dev.sh
```

With no arguments, `run-dev.sh` builds and opens the UI on that sample data.
Any arguments are passed to `subsite-monitor`, allowing CLI commands or a
different log path.
