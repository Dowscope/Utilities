# Alpha Release Packaging

The current development target is alpha.7. Alpha.6 is the latest published
binary release. The alpha.7 archive and checksum have been built locally and
are ready to attach to the `v0.1.0-alpha.7` GitHub release.
The archive is `packaging/dist/subsite-monitor-0.1.0-alpha.7-linux-x86_64-gnu.tar.gz`
with SHA-256 `b824aa354ac7b7ff7e7940f5d077791cfd7f4ceeda0b148ad646f518fc405d23`.

The published alpha.5 package included configured-site fallback and continuous
CSV collection. The alpha.7 build targets x86_64 Linux. The executable statically links
ncursesw, libstdc++, libgcc, and glibc; the UI still uses host terminfo. Static
glibc binaries have portability tradeoffs. This build has only been exercised
on Arch; validate it on each intended server before production use. The static
ncurses build searches `/etc/terminfo`, `/lib/terminfo`, and
`/usr/share/terminfo` on the target host.

Build the archive and generate checksum-pinned `installer.sh` and compatibility
`install.sh` scripts with:

```sh
./packaging/build-alpha.sh
```

The script downloads the ncurses 6.5 source archive, verifies its SHA-256,
builds static ncurses libraries, runs the test suite, and links the application
statically. It produces
`packaging/dist/subsite-monitor-0.1.0-alpha.5-linux-x86_64-gnu.tar.gz` and a
relative-path `SHA256SUMS`. The archive includes the executable, user manual,
example config, an Nginx `conf.d` snippet, a logrotate policy, and the systemd
unit. The installer creates `/etc/subsite-monitor/monitor.conf` from the
example on first install, with root ownership, mode 0600, and a mode 0700
directory. It creates private state and log directories, installs the snippet
under `/etc/nginx/conf.d`, the rotation policy under `/etc/logrotate.d`, and the
service unit under `/usr/lib/systemd/system`. It migrates the previous default
log path while preserving custom config and existing snippet/rotation files.
It does not edit the main Nginx config or start an inactive Nginx service.
When Nginx is installed, it runs `nginx -t`, verifies the snippet appears in
`nginx -T`, and reloads `nginx.service` when active. It enables and starts
the monitor service when the configured log is readable, and restarts an
active monitor on upgrade. `make install` only installs files and does not
test/reload Nginx or enable/start the monitor service.
For pre-release testing, pass the matching local archive as
`SUBSITE_MONITOR_ARCHIVE=/path/to/archive`; the installer still checks its
embedded SHA-256 and does not require the GitHub release asset to exist.
`SUBSITE_MONITOR_ACCESS_LOG=/absolute/path` can select a custom log path on
first install; the value is written into the default monitor config, Nginx
snippet, and logrotate policy. Existing custom config values remain unchanged.

For GitHub release `v0.1.0-alpha.5`, use tag `v0.1.0-alpha.5` and attach the generated
archive and `SHA256SUMS`. The installer downloads the archive from that release
tag and checks its embedded SHA-256 before installation. Upload the matching archive and `SHA256SUMS` before publishing the release; the installer will fail until those assets are attached. It supports Debian,
Ubuntu, and Arch on x86_64. The binary installs to `/usr/local/bin` by default;
set `SUBSITE_MONITOR_INSTALL_DIR` for another executable destination. Set
`SUBSITE_MONITOR_CONFIG` to customize the config destination when installing.

The root installer script is published from
`https://raw.githubusercontent.com/Dowscope/Utilities/main/Linux/SubsiteMonitor/installer.sh`.
The script URL reflects pushed source changes. The published alpha.5 archive
has SHA-256
`c2ce6821abe4b95afce815e60c9afb59c1606fdf3812783ff5baf5541ee0235c`; the
current Git installer is pinned to that artifact. The local rebuild includes
refreshed documentation and has a different checksum. Replacing the published
archive requires updating `SHA256SUMS` and the installer pin together.

The alpha.7 source adds sortable site columns, defaulting to latest access
first. The installer template lists the ten newest compatible
GitHub releases, lets interactive users choose a version, defaults to the
newest in non-interactive use, and offers `--latest`, `--list`, and
`--version <version>`. It requires Python 3 to parse GitHub's release API
response and curl or wget for downloads. It only offers releases with the
expected x86_64 archive, `SHA256SUMS`, and valid SHA-256 metadata.

The alpha.7 archive, checksum, and standalone installer are prepared by
`./packaging/build-alpha.sh`. Create the GitHub release as
`v0.1.0-alpha.7`, then attach the matching archive and `SHA256SUMS` from
`packaging/dist/`. Until those assets are uploaded, the installer will keep
offering alpha.6 as the newest installable release; a tag alone is not
installable.

Suggested release title:

> Subsite Monitor 0.1.0-alpha.7 — Sortable Site Activity

Suggested release notes:

> - The site list now shows the most recently active websites first by default.
> - Sort sites by website, request count, or last access with keys `1`–`3`;
>   press the same key again to reverse the sort.
> - Request history opens with the newest records first and sorts the displayed
>   page by timestamp, IP, method/status, or URL.
> - The installer continues to list compatible releases and verify archive
>   SHA-256 values before installation.
> - Release binaries target x86_64 Linux; the terminal UI uses system terminfo.

The build host is Arch Linux. No Debian or Ubuntu environment is available in
this checkout, so validate on those systems before production use.
