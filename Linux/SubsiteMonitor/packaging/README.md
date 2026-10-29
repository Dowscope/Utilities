# Alpha Release Packaging

The development target is **0.1.0-alpha.12**. Alpha.11 is the latest
published binary release. Alpha.12 sorts compatible releases by semantic
version, newest first, in the installer picker and in-UI updater. GitHub's API
order can differ from version order, so alpha.11 appeared below older releases.

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

- `packaging/dist/subsite-monitor-0.1.0-alpha.12-linux-x86_64-gnu.tar.gz`
- `packaging/dist/SHA256SUMS`
- checksum-pinned `installer.sh` and compatibility `install.sh`

The alpha.12 archive SHA-256 is
`8ba1e0bf110a6d689bea3901a879575182667c660580cd350f6583f65d955ffc`.
`./verify.sh` and `./packaging/build-alpha.sh` passed without compiler
warnings. The archive checksum verified successfully.

The archive contains the executable, README, user manual, example configs,
Nginx log-format snippet, logrotate policy, and systemd unit. The installer
creates the root-only config on first install, private state/log directories,
and package integration files. It preserves existing config and historical
statistics, validates and reloads active Nginx when configured, and enables or
restarts the monitor service when the access log is readable. It never edits
the main Nginx config or starts an inactive Nginx service.

For local testing before publication, pass the archive directly:

```sh
sudo env SUBSITE_MONITOR_ARCHIVE="$PWD/packaging/dist/subsite-monitor-0.1.0-alpha.12-linux-x86_64-gnu.tar.gz" ./installer.sh
```

The installer still checks the archive's embedded checksum. Set
`SUBSITE_MONITOR_ACCESS_LOG=/absolute/path` to choose the default log path on a
fresh installation; existing custom configuration is retained.

## Publish the release

Create tag `v0.1.0-alpha.12` and a GitHub Release with this title:

> Subsite Monitor 0.1.0-alpha.12 — Correct Release Ordering

Release notes:

> - Installer release selection now sorts valid releases by semantic version,
>   placing the newest version first regardless of GitHub API ordering.
> - The installer still limits its interactive list to the 10 newest compatible
>   releases and selects the newest compatible release by default.
> - The in-UI updater release list now uses the same newest-first ordering.
> - Added tests for numeric prerelease sorting and default latest selection.
> - Release binaries target x86_64 Linux and use host terminfo.

Attach the matching alpha.12 archive and `SHA256SUMS` from `packaging/dist/`.
The generated installer is pinned to this archive and will not offer alpha.12
as an installable update until both assets are attached to the GitHub Release.

The tracked installer URL is
`https://raw.githubusercontent.com/Dowscope/Utilities/main/Linux/SubsiteMonitor/installer.sh`.
It reflects this source update after the commit is pushed. No server deployment
is performed by the packaging step.
