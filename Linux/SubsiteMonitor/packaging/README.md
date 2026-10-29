# Alpha Release Packaging

The development target is **0.1.0-alpha.9**. Alpha.8 is the latest published
binary release. Alpha.9 updates the UI command bars to grouped `Action: key`
labels and shows the update indicator only when a newer compatible release is
available.

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

- `packaging/dist/subsite-monitor-0.1.0-alpha.9-linux-x86_64-gnu.tar.gz`
- `packaging/dist/SHA256SUMS`
- checksum-pinned `installer.sh` and compatibility `install.sh`

The alpha.9 archive SHA-256 is
`43afe61193201337797db932b96d23033f77c83d5f1f2d928d2a4fb83497cd50`.
`./verify.sh` and `./packaging/build-alpha.sh` passed without compiler
warnings. The release build's static tests and installer checks passed too.

The archive contains the executable, README, user manual, example configs,
Nginx log-format snippet, logrotate policy, and systemd unit. The installer
creates the root-only config on first install, private state/log directories,
and package integration files. It preserves existing config and historical
statistics, validates and reloads active Nginx when configured, and enables or
restarts the monitor service when the access log is readable. It never edits
the main Nginx config or starts an inactive Nginx service.

For local testing before publication, pass the archive directly:

```sh
sudo env SUBSITE_MONITOR_ARCHIVE="$PWD/packaging/dist/subsite-monitor-0.1.0-alpha.9-linux-x86_64-gnu.tar.gz" ./installer.sh
```

The installer still checks the archive's embedded checksum. Set
`SUBSITE_MONITOR_ACCESS_LOG=/absolute/path` to choose the default log path on a
fresh installation; existing custom configuration is retained.

## Publish the release

Create tag `v0.1.0-alpha.9` and a GitHub Release with this title:

> Subsite Monitor 0.1.0-alpha.9 — Clearer UI Commands and Update Notices

Release notes:

> - Reworked overview, request-history, and update-review command bars into
>   `Action: key | Action: key` groups while retaining the existing colors.
> - Added compact command labels for narrower terminals so the available
>   actions remain visible.
> - The lower-right update indicator now appears only when a newer compatible
>   release is available.
> - The installer verifies the selected release archive's SHA-256 before
>   installation. It restarts the monitor service on upgrade and reloads
>   active Nginx when applicable.
> - Release binaries target x86_64 Linux and use host terminfo.

Attach the matching alpha.9 archive and `SHA256SUMS` from `packaging/dist/`.
The generated installer is pinned to this archive and will not find alpha.9 as
an installable release until both assets are attached to the GitHub Release.

The tracked installer URL is
`https://raw.githubusercontent.com/Dowscope/Utilities/main/Linux/SubsiteMonitor/installer.sh`.
It reflects this source update after the commit is pushed. No server deployment
is performed by the packaging step.
