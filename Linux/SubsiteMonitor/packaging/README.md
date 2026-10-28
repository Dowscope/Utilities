# Alpha Release Packaging

`0.1.0-alpha.5` is the current published alpha and includes configured-site
fallback and continuous CSV collection. The
alpha.5 build targets x86_64 Linux. The executable statically links
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
It does not edit the main Nginx config or enable/start the monitor service.
When Nginx is installed, it runs `nginx -t`, verifies the snippet appears in
`nginx -T`, and reloads `nginx.service` when active. It does not start an
inactive Nginx service. `make install` only installs the files and does not
test or reload Nginx.
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

The build host is Arch Linux. No Debian or Ubuntu environment is available in
this checkout, so validate on those systems before production use.
