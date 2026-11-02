# Alpha Release Packaging

The release candidate is **0.1.0-alpha.17**. Alpha.13 is the latest
published binary release. The alpha.15 archive adds per-site request counts
and is built and checksum-verified. Alpha.17 adds five-second live refreshes
and three-state sorting; its x86_64 archive is built and checksum-verified.

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

- `packaging/dist/subsite-monitor-0.1.0-alpha.17-linux-x86_64-gnu.tar.gz`
- `packaging/dist/SHA256SUMS`
- checksum-pinned `installer.sh` and compatibility `install.sh`

The alpha.17 build and full test suite passed without compiler warnings. Its
archive checksum in `packaging/dist/SHA256SUMS` is
`983e423a7234d6add4cf4a7a26aa8866473ca4ba3e08cfb28c9cbbc893a88c3e`. The
site table defaults to today; `p` cycles per-site counts through calendar
windows and an exact rolling 24-hour count. The overview refreshes
automatically every five seconds; `r` refreshes immediately.

The archive contains the executable, README, user manual, example configs,
Nginx log-format snippet, logrotate policy, and systemd unit. The installer
creates the root-only config on first install, private state/log directories,
and package integration files. It preserves existing config and historical
statistics, validates and reloads active Nginx when configured, and enables or
restarts the monitor service when the access log is readable. It never edits
the main Nginx config or starts an inactive Nginx service.

For local testing before publication, pass the archive directly:

```sh
sudo env SUBSITE_MONITOR_ARCHIVE="$PWD/packaging/dist/subsite-monitor-0.1.0-alpha.17-linux-x86_64-gnu.tar.gz" ./installer.sh
```

The installer still checks the archive's embedded checksum. Set
`SUBSITE_MONITOR_ACCESS_LOG=/absolute/path` to choose the default log path on a
fresh installation; existing custom configuration is retained.

## Publish the release

Create tag `v0.1.0-alpha.17` and a GitHub Release with this title:

> Subsite Monitor 0.1.0-alpha.17 — Sortable Views and Live Activity

Release notes:

> - Added per-site request counts for a selectable period, defaulting to today.
> - Press `p` to cycle through 1 year, 6 months, 90 days, 30 days, 7 days,
>   and an exact rolling 24 hours.
> - Sort columns through ascending, descending, then the default order by
>   pressing the active column key repeatedly.
> - The site overview refreshes every five seconds; press `r` for an immediate
>   refresh.
> - The UI status bar reports whether the monitor service lock is active.
> - Release binaries target x86_64 Linux and use host terminfo.
>
> SHA-256: `983e423a7234d6add4cf4a7a26aa8866473ca4ba3e08cfb28c9cbbc893a88c3e`

Attach the matching alpha.17 archive and `SHA256SUMS` from `packaging/dist/`.
The packaging script generates an installer pinned to this archive. The
updater will not offer alpha.17 until the archive and checksum are attached to
the GitHub Release.

After the source commit is pushed, the tracked installer URL is
`https://raw.githubusercontent.com/Dowscope/Utilities/main/Linux/SubsiteMonitor/installer.sh`.
It reflects this source update after the commit is pushed. No server deployment
is performed by the packaging step.
