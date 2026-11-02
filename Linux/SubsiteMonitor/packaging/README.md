# Alpha Release Packaging

The development target is **0.1.0-alpha.14**. Alpha.13 is the latest
published binary release. Alpha.14 adds a bottom status bar for the UI and the
monitoring service. Its x86_64 release archive and checksum have been built and
verified.

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

- `packaging/dist/subsite-monitor-0.1.0-alpha.14-linux-x86_64-gnu.tar.gz`
- `packaging/dist/SHA256SUMS`
- checksum-pinned `installer.sh` and compatibility `install.sh`

The alpha.14 archive checksum is recorded in `packaging/dist/SHA256SUMS`. The
status bar reports the UI as running and checks whether the monitor service
holds its exclusive lock every two seconds. It shows `ACTIVE`, `STOPPED`, or
`UNKNOWN`; an active lock confirms ownership but does not prove that requests
are arriving or that the service is making progress.
The archive SHA-256 is
`9229f199e9d74878d211f1e9d1c3659b56dabbcee4cf7dc1cdd1ae285fe06f8f`.

The archive contains the executable, README, user manual, example configs,
Nginx log-format snippet, logrotate policy, and systemd unit. The installer
creates the root-only config on first install, private state/log directories,
and package integration files. It preserves existing config and historical
statistics, validates and reloads active Nginx when configured, and enables or
restarts the monitor service when the access log is readable. It never edits
the main Nginx config or starts an inactive Nginx service.

For local testing before publication, pass the archive directly:

```sh
sudo env SUBSITE_MONITOR_ARCHIVE="$PWD/packaging/dist/subsite-monitor-0.1.0-alpha.14-linux-x86_64-gnu.tar.gz" ./installer.sh
```

The installer still checks the archive's embedded checksum. Set
`SUBSITE_MONITOR_ACCESS_LOG=/absolute/path` to choose the default log path on a
fresh installation; existing custom configuration is retained.

## Publish the release

Create tag `v0.1.0-alpha.14` and a GitHub Release with this title:

> Subsite Monitor 0.1.0-alpha.14 — UI and Monitor Health Status

Release notes:

> - Added a bottom status bar showing that the UI is running and the monitor
>   service state.
> - The monitor service status refreshes every two seconds and shows active,
>   stopped, or unknown based on its exclusive service lock.
> - The updater preserves the original UI arguments. Users updating directly
>   from alpha.12 must close and reopen the UI once; alpha.12 cannot run the
>   automatic restart behavior introduced in alpha.13.
> - Release binaries target x86_64 Linux and use host terminfo.

Attach the matching alpha.14 archive and `SHA256SUMS` from `packaging/dist/`.
The generated installer is pinned to this archive and will not offer alpha.14
as an installable update until both assets are attached to the GitHub Release.

The tracked installer URL is
`https://raw.githubusercontent.com/Dowscope/Utilities/main/Linux/SubsiteMonitor/installer.sh`.
It reflects this source update after the commit is pushed. No server deployment
is performed by the packaging step.
