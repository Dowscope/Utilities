# Alpha Release Packaging

The development target is **0.1.0-alpha.13**. Alpha.12 is the latest
published binary release. Alpha.13 automatically restarts the UI from the
newly installed executable after a successful update, preserving the original
UI arguments.

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

- `packaging/dist/subsite-monitor-0.1.0-alpha.13-linux-x86_64-gnu.tar.gz`
- `packaging/dist/SHA256SUMS`
- checksum-pinned `installer.sh` and compatibility `install.sh`

The alpha.13 archive SHA-256 is
`d6378c9595ef664692df63c12f4f364388fa2ea7d7d0cd8c836d471ec8baf7f3`.
`./verify.sh` and `./packaging/build-alpha.sh` passed without compiler
warnings. The archive checksum verified successfully. A pseudo-terminal test
also confirmed the UI re-executes the installed binary and preserves `ui --log`
arguments after a successful update.

The archive contains the executable, README, user manual, example configs,
Nginx log-format snippet, logrotate policy, and systemd unit. The installer
creates the root-only config on first install, private state/log directories,
and package integration files. It preserves existing config and historical
statistics, validates and reloads active Nginx when configured, and enables or
restarts the monitor service when the access log is readable. It never edits
the main Nginx config or starts an inactive Nginx service.

For local testing before publication, pass the archive directly:

```sh
sudo env SUBSITE_MONITOR_ARCHIVE="$PWD/packaging/dist/subsite-monitor-0.1.0-alpha.13-linux-x86_64-gnu.tar.gz" ./installer.sh
```

The installer still checks the archive's embedded checksum. Set
`SUBSITE_MONITOR_ACCESS_LOG=/absolute/path` to choose the default log path on a
fresh installation; existing custom configuration is retained.

## Publish the release

Create tag `v0.1.0-alpha.13` and a GitHub Release with this title:

> Subsite Monitor 0.1.0-alpha.13 — Automatic UI Restart After Update

Release notes:

> - After a verified update, the UI automatically restarts into the newly
>   installed version instead of leaving the old version running.
> - The restart preserves the original UI command-line arguments.
> - If the installed executable cannot be restarted, the UI reports the error
>   and explains how to relaunch it manually.
> - Release binaries target x86_64 Linux and use host terminfo.

Attach the matching alpha.13 archive and `SHA256SUMS` from `packaging/dist/`.
The generated installer is pinned to this archive and will not offer alpha.13
as an installable update until both assets are attached to the GitHub Release.

The tracked installer URL is
`https://raw.githubusercontent.com/Dowscope/Utilities/main/Linux/SubsiteMonitor/installer.sh`.
It reflects this source update after the commit is pushed. No server deployment
is performed by the packaging step.
