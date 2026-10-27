# Alpha Release Packaging

`0.1.0-alpha.2` is the latest published alpha. Alpha.3 adds Nginx
`server_name` discovery so configured virtual hosts without logged requests are
listed in the UI. The alpha.3 build targets x86_64 Linux. The executable statically links
ncursesw, libstdc++, libgcc, and glibc; the UI still uses host terminfo. Static
glibc binaries have portability tradeoffs. This build has only been exercised
on Arch; validate it on each intended server before production use.

Build the archive and generate checksum-pinned `installer.sh` and compatibility
`install.sh` scripts with:

```sh
./packaging/build-alpha.sh
```

The script downloads the ncurses 6.5 source archive, verifies its SHA-256,
builds static ncurses libraries, runs the test suite, and links the application
statically. It produces
`packaging/dist/subsite-monitor-0.1.0-alpha.3-linux-x86_64-gnu.tar.gz` and a
relative-path `SHA256SUMS`. The archive includes the executable, user manual,
and example config. The installer creates `/etc/subsite-monitor/monitor.conf`
from that example on first install, with root ownership, mode 0600, and a
mode 0700 directory. Existing config is preserved on upgrades and its
permissions are tightened. The installer does not change Nginx configuration.

For GitHub release `v0.1.0-alpha.3`, use tag `v0.1.0-alpha.3` and attach the generated
archive and `SHA256SUMS`. The installer downloads the archive from that release
tag and checks its embedded SHA-256 before installation. Upload the matching archive and `SHA256SUMS` before publishing the release; the installer will fail until those assets are attached. It supports Debian,
Ubuntu, and Arch on x86_64. The binary installs to `/usr/local/bin` by default;
set `SUBSITE_MONITOR_INSTALL_DIR` for another executable destination. Set
`SUBSITE_MONITOR_CONFIG` to customize the config destination when installing.

The root installer script is published from
`https://raw.githubusercontent.com/Dowscope/Utilities/main/Linux/SubsiteMonitor/installer.sh`.
The script URL is usable after source changes are pushed; installation also
requires the matching release archive to be attached.

The build host is Arch Linux. No Debian or Ubuntu environment is available in
this checkout, so validate on those systems before production use.
