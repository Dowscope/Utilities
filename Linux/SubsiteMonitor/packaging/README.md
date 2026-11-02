# Alpha Release Packaging

The development target is **0.1.0-alpha.15**. Alpha.13 is the latest
published binary release. The alpha.15 release candidate adds per-site request
counts for a selected time period. Its x86_64 archive is built and
checksum-verified; the release has not yet been published.

## Build the release package

Run:

```sh
./verify.sh
./packaging/build-alpha.sh
```

The packaging script supports x86_64 Linux. It downloads the pinned ncurses
6.5 source archive when needed, checks its SHA-256, builds static ncurses
libraries, runs the full test suite, and links a static executable. The binary
uses the host terminfo database. Static glibc binaries have portability
tradeoffs; test on each target distribution before production use.

The build produces:

- `packaging/dist/subsite-monitor-0.1.0-alpha.15-linux-x86_64-gnu.tar.gz`
- `packaging/dist/SHA256SUMS`
- checksum-pinned `installer.sh` and compatibility `install.sh`

Alpha.15's archive checksum is recorded in `packaging/dist/SHA256SUMS`:
`36ff0a5b064606188c4a56f48b112c005ff6a19cc0b6d3168387dd2a232837b7`. The
site table defaults to today; `p` cycles the per-site counts through calendar
windows and an exact rolling 24-hour count.

The archive contains the executable, README, user manual, example configs,
Nginx log-format snippet, logrotate policy, and systemd unit. The installer
creates the root-only config on first install, private state/log directories,
and package integration files. It preserves existing config and historical
statistics, validates and reloads active Nginx when configured, and enables or
restarts the monitor service when the access log is readable. It never edits
the main Nginx config or starts an inactive Nginx service.

For local testing before publication, pass the archive directly:

```sh
sudo env SUBSITE_MONITOR_ARCHIVE="$PWD/packaging/dist/subsite-monitor-0.1.0-alpha.15-linux-x86_64-gnu.tar.gz" ./installer.sh
```

The installer still checks the archive's embedded checksum. Set
`SUBSITE_MONITOR_ACCESS_LOG=/absolute/path` to choose the default log path on a
fresh installation; existing custom configuration is retained.

## Publish the release

Create tag `v0.1.0-alpha.15` and a GitHub Release with this title:

> Subsite Monitor 0.1.0-alpha.15 — Request Activity Time Ranges

Release notes:

> - Added per-site request counts for a selectable period, defaulting to today.
> - Press `p` to cycle through 1 year, 6 months, 90 days, 30 days, 7 days,
>   and an exact rolling 24 hours; sorting follows the selected period.
> - Press `r` to refresh the per-site counts.
> - The UI status bar reports whether the monitor service lock is active.
> - Release binaries target x86_64 Linux and use host terminfo.

Attach the matching alpha.15 archive and `SHA256SUMS` from `packaging/dist/`.
The generated installer is pinned to this archive and will not offer alpha.15
as an installable update until both assets are attached to the GitHub Release.

After the source commit is pushed, the tracked installer URL is
`https://raw.githubusercontent.com/Dowscope/Utilities/main/Linux/SubsiteMonitor/installer.sh`.
It reflects this source update after the commit is pushed. No server deployment
is performed by the packaging step.
