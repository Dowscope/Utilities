# Alpha Release Packaging

The current development target is alpha.8. Alpha.6 is the latest published
binary release. Alpha.8 adds an in-UI release updater. The archive and checksum
were built and verified after the final test suite passed:
`packaging/dist/subsite-monitor-0.1.0-alpha.8-linux-x86_64-gnu.tar.gz`, SHA-256
`f565f158c1fe8fb21ce0084f74e8c8aca4f7a15b0b1a873437575115c490fe8c`.

The published alpha.5 package included configured-site fallback and continuous
CSV collection. The alpha.8 build targets x86_64 Linux. The executable statically links
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

The alpha.8 source adds sortable site columns, defaulting to latest access
first, and an update action in the UI. The updater lists newer compatible
releases with their notes and requires explicit confirmation before invoking
the installed installer. The installer template lists the ten newest compatible
GitHub releases, lets interactive users choose a version, defaults to the
newest in non-interactive use, and offers `--latest`, `--list`, and
`--version <version>`. It requires Python 3 to parse GitHub's release API
response and curl or wget for downloads. It only offers releases with the
expected x86_64 archive, `SHA256SUMS`, and valid SHA-256 metadata.

The alpha.8 archive, checksum, and standalone installer are prepared by
`./packaging/build-alpha.sh`. Create the GitHub release as
`v0.1.0-alpha.8`, then attach the matching archive and `SHA256SUMS` from
`packaging/dist/`. Until those assets are uploaded, the installer will keep
offering alpha.6 as the newest installable release; a tag alone is not
installable.

Suggested release title:

> Subsite Monitor 0.1.0-alpha.8 — In-UI Software Updates

Suggested release notes:

> - Added a UI updater that lists newer compatible releases with their release
>   notes. Installation requires explicit confirmation.
> - The updater uses the installed release installer, which verifies archive
>   SHA-256 before replacing the executable.
> - The installer refreshes its installed update helper during installation.
> - Site activity defaults to most recently active first; site and request
>   tables remain sortable by their displayed columns.
> - Release binaries target x86_64 Linux; the terminal UI uses system terminfo.

The build host is Arch Linux. No Debian or Ubuntu environment is available in
this checkout, so validate on those systems before production use.
