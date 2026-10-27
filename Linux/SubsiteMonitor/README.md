# Subsite Monitor

Next alpha under development: **0.1.0-alpha.2**. The published alpha is
**0.1.0-alpha.1**. Release binaries target x86_64 Linux on Debian, Ubuntu, and
Arch, statically link ncursesw and the C++ runtime, and use host terminfo for
the terminal UI.

A small C++ command-line utility for monitoring website activity from web-server
access logs. It streams the documented Nginx log format, reports per-site and
daily statistics, and provides an ncurses screen. Its default UI uses a
root-only access-log configuration. CSV history and persistent checkpoints
remain planned work.

## Build and test

Requirements: GCC with C++23 support, GNU Make, `pkg-config`, and ncursesw
development files.

```sh
make -j
make test
```

Use `make clean` to remove build outputs, `make debug` for a debug build, and
`make install` to install under `/usr/local` and seed the root-only config at
`/etc/subsite-monitor/monitor.conf` (override `PREFIX`, `SYSCONFDIR`, or use
`DESTDIR` for packaging).

For local development, `./run-dev.sh` builds and opens the ncurses UI with the
bundled sample log. Pass arguments to run another command or log. Run
`./verify.sh` for a clean build, tests, and CLI smoke checks.
The [user and technical manuals](Docs/README.md) describe current behavior and
design. [Prompt.md](Prompt.md) preserves the project handoff brief,
[Changelog.md](Changelog.md) records changes, and [TODO.md](TODO.md) tracks
approved future work.

`./verify.sh` passed on 2026-10-27T14:22:35-04:00 with a clean C++23 build,
parser/statistics/config/CLI tests, help, and version checks. The default UI
was exercised in a pseudo-terminal and displayed 2 sites from the fixture.
See `packaging/README.md` for alpha build and release steps.

## Current CLI

```text
subsite-monitor --help
subsite-monitor --version
sudo subsite-monitor
subsite-monitor collect --log /path/to/access.log
subsite-monitor stats --log /path/to/access.log
sudo subsite-monitor ui
subsite-monitor ui --log /path/to/access.log
```

`collect` prints the number of parsed and malformed lines. It does not print
request targets or client addresses. See the [manual](Docs/MANUAL.md) for the
supported Nginx format, parser limits, and statistics report. The current
`stats` command reads one log directly, with caps of 10,000 distinct websites
and 100,000 website/date pairs; CSV-backed history is a later step.

With no arguments, the application opens the interactive UI using the
root-only configuration. It defaults to
`/var/log/nginx/subsite-monitor-access.log`; edit the config with
`sudoedit /etc/subsite-monitor/monitor.conf`. The log must use the documented
host-aware Nginx format. The UI counts distinct hostnames with valid requests
in that log; sites with no valid entries cannot be inferred from access logs
alone. Use `SUBSITE_MONITOR_LOG` to override the log path, or
`SUBSITE_MONITOR_CONFIG` to select another root-only config.

The `ui` command uses the same configuration, or accepts `--log <path>` as an
override. Use the arrow keys or `j`/`k` to select a site, `r` to reread the log,
and `q` to quit. It requires a real interactive terminal. For sample data,
run:

```sh
./run-dev.sh
```

With no arguments, `run-dev.sh` builds and opens the UI on that sample data.
Any arguments are passed to `subsite-monitor`, allowing CLI commands or a
different log path.

## Alpha installer

The installer and release build instructions are in `installer.sh` and
`packaging/README.md`. The upcoming alpha.2 binary targets Debian, Ubuntu, and
Arch on x86_64. Its installer seeds and preserves the root-only monitor config
but does not install system packages or change Nginx configuration.
