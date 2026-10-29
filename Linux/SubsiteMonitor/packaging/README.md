# Alpha Release Packaging

The development target is **0.1.0-alpha.10**. Alpha.9 is the latest published
binary release. Alpha.10 adds the running version to the overview and
request-history headers, and provides a direct GitHub Issues link in the
overview and CLI help. The existing command-bar groups and update indicator
behavior are retained.

The alpha.10 archive, checksum, and checksum-pinned installer are built from
this source. Do not replace the published alpha.9 assets.

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

- `packaging/dist/subsite-monitor-0.1.0-alpha.10-linux-x86_64-gnu.tar.gz`
- `packaging/dist/SHA256SUMS`
- checksum-pinned `installer.sh` and compatibility `install.sh`

The alpha.10 archive SHA-256 is
`6f23a76d8bbb9aa9c9c449eeab57f1c3576ea304c5c70cd518584620b15c28b0`.
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
sudo env SUBSITE_MONITOR_ARCHIVE="$PWD/packaging/dist/subsite-monitor-0.1.0-alpha.10-linux-x86_64-gnu.tar.gz" ./installer.sh
```

The installer still checks the archive's embedded checksum. Set
`SUBSITE_MONITOR_ACCESS_LOG=/absolute/path` to choose the default log path on a
fresh installation; existing custom configuration is retained.

## Publish the release

Create tag `v0.1.0-alpha.10` and a GitHub Release with this title:

> Subsite Monitor 0.1.0-alpha.10 — Version and Support Details

Release notes:

> - Added the running application version to the overview and request-history
>   headers.
> - Added a direct GitHub Issues URL to the site overview and CLI help output.
> - Updated UI and CLI checks and user/technical documentation for the new
>   version and support details.
> - Release binaries target x86_64 Linux and use host terminfo.

Attach the matching alpha.10 archive and `SHA256SUMS` from `packaging/dist/`.
The generated installer is pinned to this archive and will not find alpha.10 as
an installable release until both assets are attached to the GitHub Release.

The tracked installer URL is
`https://raw.githubusercontent.com/Dowscope/Utilities/main/Linux/SubsiteMonitor/installer.sh`.
It reflects this source update after the commit is pushed. No server deployment
is performed by the packaging step.
