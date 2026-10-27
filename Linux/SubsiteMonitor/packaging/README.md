# Alpha Release Packaging

The first alpha is `0.1.0-alpha.1`, built for x86_64 Linux. The executable
statically links ncursesw, libstdc++, libgcc, and glibc, so it does not need
matching runtime shared-library versions on Debian, Ubuntu, or Arch. The UI
uses the host's terminfo database. Static glibc binaries have portability
tradeoffs and this artifact has only been built and exercised on Arch; test it
on each intended server before relying on it.

Build the archive and generate the checksum-pinned `installer.sh` and legacy
`install.sh` entry points with:

```sh
./packaging/build-alpha.sh
```

The script downloads the official ncurses 6.5 source archive, verifies its
SHA-256, builds the required static ncurses libraries, runs the project test
suite, and links the release binary statically. It creates
`packaging/dist/subsite-monitor-0.1.0-alpha.1-linux-x86_64-gnu.tar.gz` and
`SHA256SUMS`, embeds the artifact checksum in both installer scripts, and tests
normal installation plus rejection of a corrupted archive. Generated
artifacts and downloaded source under `packaging/dist/` are not committed.

For a GitHub release, create tag `v0.1.0-alpha.1` and attach the generated
archive and `SHA256SUMS`. The scripts download the archive from that release
tag and refuse installation if its embedded checksum does not match. The
installer supports Debian, Ubuntu, and Arch on x86_64. It installs to
`/usr/local/bin` by default; set `SUBSITE_MONITOR_INSTALL_DIR` to use another
directory. It does not install packages or change Nginx configuration or
existing configuration/data files.

The scripts are published from the repository root at
`https://raw.githubusercontent.com/Dowscope/Utilities/main/Linux/SubsiteMonitor/installer.sh`.
That URL is usable after the source changes are pushed. A working download
installer additionally requires the release archive to be attached to the
GitHub release, as described above.

No Debian or Ubuntu environment is available in this development checkout, so
the binary has not been executed there. Static glibc linking does not guarantee
that every terminal, kernel, or terminfo setup will work; validate on target
distributions before production use. Do not use an Arch-built *dynamic* binary
as a substitute release artifact.
