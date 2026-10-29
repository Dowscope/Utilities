# Subsite Monitor User Manual

## What the program does

Subsite Monitor helps identify which hosted websites receive requests by
reading existing web-server access logs. It reads the supported
Nginx access-log format, counts valid and malformed lines, and displays
per-site statistics in an ncurses terminal screen. Collection stores a daily
summary CSV and a separate CSV containing every valid request. Enter on a site
opens its actual request records, including the full URL and client IP. The
default UI uses a root-only config file.

## Requirements

- Linux or another environment with GNU Make and GCC.
- A GCC version with C++23 support.
- GNU `pkg-config` and ncursesw development files for the terminal UI.

On Debian or Ubuntu, install `g++ make pkg-config libncurses-dev`. On Arch,
install `gcc make pkgconf ncurses`.

The alpha release installer supports x86_64 Debian, Ubuntu, and Arch systems.
Its binary statically links the runtime libraries; the terminal UI still needs
a usable terminfo database. The development build requirements above are not
needed to run the installed release executable. The UI updater needs Python 3
and network access to GitHub to read the compatible release list.

## Build and test

From the project directory:

```sh
make -j
make test
```

`make clean` removes the executable and generated object/dependency files.
`make debug` builds with debug symbols and no optimization. `make install`
installs the executable under `/usr/local/bin` and seeds
`/etc/subsite-monitor/monitor.conf` with mode 0600. Set `PREFIX` or
`SYSCONFDIR` to change paths, or set `DESTDIR` to stage installation. It also
installs `subsite-monitor.service` under `/usr/lib/systemd/system`; installation
creates `/var/lib/subsite-monitor` with mode 0700, and does not enable or start
the service. It installs `/etc/nginx/conf.d/subsite-monitor.conf` and
`/etc/logrotate.d/subsite-monitor`; `make install` does not test or reload
Nginx. The snippet loads only if the main config includes `conf.d/*.conf`
inside `http { }`. Set `STATEDIR` to change the state root.

The helper scripts provide common local workflows:

- `./run-dev.sh` builds as needed and runs the program. With no arguments it
  opens the ncurses UI using `tests/fixtures/access.log` and the matching Nginx
  config; pass CLI arguments to run a specific command.
- `./verify.sh` performs a clean build, runs parser, storage, and CLI tests, and checks
  help and version output. It includes tests for config parsing and default UI
  dispatch.
- `./run-docker.sh` starts a local Nginx test server with Docker Compose. Docker
  Engine and Compose v2 must be installed. It serves three local virtual hosts
  and writes host-aware requests to `docker-data/logs/access.log`.

### Testing against the Docker Nginx server

Use two terminals. Start Nginx in the first:

```sh
sudo ./run-docker.sh
```

In the second, send requests to two sites, leaving `inactive.test` untouched,
then launch the development build against the files shared from the container:

```sh
curl -H 'Host: alpha.test' http://127.0.0.1:8080/
curl -H 'Host: beta.test' http://127.0.0.1:8080/
./run-dev.sh
```

The UI should show `alpha.test` and `beta.test` with requests, and
`inactive.test` with zero requests. Press `r` to refresh after sending more
requests from another terminal. With no environment overrides, `run-dev.sh`
automatically uses `docker-data/logs/access.log` and
`docker/nginx/nginx.conf` when the Docker log exists; otherwise it uses the
sample fixtures. `docker-data/` is ignored by Git. Stop Nginx with Ctrl-C in
the first terminal; `sudo ./run-docker.sh down` also stops it.

The Compose service binds only to localhost and mounts the Nginx config
read-only. The application itself builds and runs natively on the development
machine, reading the same config and log files that the container uses.

### Testing with copied server data

For a server snapshot, copy only the required Nginx config and selected log
files into a private local directory. Logs can contain sensitive URLs and
client addresses; Nginx directories may contain private keys. Review copied
files, exclude secrets, and do not place them in Git. Keep include paths
resolvable. Then run the development app with paths to the copy:

```sh
SUBSITE_MONITOR_LOG=/absolute/path/to/copy/access.log \
SUBSITE_MONITOR_NGINX_CONFIG=/absolute/path/to/copy/nginx.conf \
  ./run-dev.sh
```

The site list comes from `server_name` in the mounted config. Per-site request
counts require the documented host-aware log format. Nginx's standard
`access.log` commonly omits the hostname, so it may show configured sites but
cannot attribute requests to them. When a configured log is missing, the
development UI shows configured sites and warns that activity is
unavailable.

## Commands currently available

```sh
subsite-monitor --help
subsite-monitor --version
sudo subsite-monitor
subsite-monitor collect --log /var/log/subsite-monitor/access.log
subsite-monitor stats
subsite-monitor stats --days 7
subsite-monitor stats --log /var/log/subsite-monitor/access.log
sudo subsite-monitor service
sudo subsite-monitor ui
subsite-monitor ui --log /var/log/subsite-monitor/access.log
```

With no arguments, the program opens the UI using the root-only config.
`collect` requires `--log <path>` and accepts `--stats-dir <path>`. It prints
parsed and malformed record counts and stores daily snapshots. Empty lines,
invalid records, and lines over 16,384 bytes count as malformed. An unreadable
or missing file returns exit status 1. Unsupported or incomplete arguments
return status 2; help, version, and a completed collection return status 0.
Starting the UI without an interactive terminal returns status 1.

`stats --log <path>` calculates a one-time report directly from that log. It
shows per-website request totals and latest access in the monitor server's local
timezone, daily counts grouped
by the date written in the log, and malformed-line count. Sites are sorted by
total requests, highest first. This direct-report mode requires `--log`. Without `--log`, `stats` reads stored
daily CSV snapshots. `--days N` selects the newest N dates that contain stored
data. Storage defaults to `/var/lib/subsite-monitor/stats` as root or the XDG
state directory for a regular user. Set `SUBSITE_MONITOR_STATS_DIR` or use
`--stats-dir <path>` to override it. Collection replaces each affected date's summary and request snapshot, so
repeating collection on the same complete log does not double count. Request
rows are stored in `requests-YYYY-MM-DD.csv` files with columns
`timestamp_utc,site,client_ip,method,url,status`. The URL includes the complete
request target, including query strings. Each accepted request gets one row;
malformed log lines are counted but are not stored as requests. The UI reads request rows in daily-file order and preserves the log order within
each day. CSV event timestamps are stored in UTC and displayed in the local
timezone configured on the machine running Subsite Monitor, with a numeric
UTC offset. The displayed URL is the complete
`$request` target (normally path plus query string); the site hostname appears
at the top of the view. The current log format does not include the URL scheme.
Arrow keys move between records, Page Up/Down loads 100-record pages, and
left/right scrolls the selected full URL.

Request CSV files contain sensitive IP addresses and full URLs, including any
query parameters. The statistics directory is private to its owner (0700), and
CSV files are owner-readable only (0600). Storage grows with the number and
length of logged requests. Keep adequate disk space available. The service uses
log checkpoints and recovery; see the continuous collection section for
rotation and truncation limitations.

The stats directory is created with mode 0700 and CSV/lock files with mode
0600. CSV replacement is atomic on the same filesystem, and collection/report
access is serialized with an advisory lock. The directory must be owned by the
current user. To keep aggregation memory bounded, one report accepts at most
10,000 distinct websites and 100,000 website/date pairs; exceeding either
limit returns an error.

## Continuous collection service

The foreground `service` command follows the configured access log and updates
the same daily summary and per-request CSV files as traffic arrives. It checks
for new complete lines about once per second. An incomplete final line is held
until its newline arrives. The service owns an exclusive lock in the statistics
directory; a second service or a manual `collect` is refused while it runs.
The UI can read the CSVs while the service is active; press `r` to refresh the
screen after new requests have been collected.

On a fresh state directory, the service starts at the current end of the log
and records future requests. The release installer enables and starts the
service when the configured log is readable; it does not backfill earlier
history. For a manual installation using `make install`, collect history
before starting the service, using the same configured log and statistics
directory:

```sh
sudo subsite-monitor collect --log /var/log/subsite-monitor/access.log
sudo systemctl daemon-reload
sudo systemctl enable --now subsite-monitor.service
sudo systemctl status subsite-monitor.service
```

Edit the root-only config before starting the unit if the log location differs
from its default. The unit runs as root so it can read protected Nginx logs and
write `/var/lib/subsite-monitor/stats`; it uses a private umask and systemd
filesystem protections. Stop it with `sudo systemctl disable --now
subsite-monitor.service`. Logs are available with
`sudo journalctl -u subsite-monitor.service`.

## Software updates in the UI

The running version appears in the top header. The site overview also displays
the project issue tracker: [https://github.com/Dowscope/Utilities/issues](https://github.com/Dowscope/Utilities/issues).
The same address appears in `subsite-monitor --help`.

When the site overview opens, Subsite Monitor checks GitHub for newer
compatible releases in the background. The lower-right indicator appears only
when an update is available and shows its version and the `u` key. No update
message appears when the installed version is current or the check fails. The
command bar stays focused on navigation; press `u` when the update indicator
appears to review releases. Python 3 and network access to GitHub are required
for the check.

Press `u` to review release titles and notes. Select a version, press Enter,
then press `y` on the confirmation screen to install it. The installer
verifies the downloaded archive checksum before replacing packaged files. It
preserves the existing configuration and statistics, refreshes the installed
installer helper, and restarts an active Subsite Monitor service. When Nginx
is installed and active, it validates the configuration and reloads Nginx.
Quit and relaunch the UI after installation so the new executable is used.

The service checkpoints the log device, inode, offset, and committed event CSV
sizes. Event rows are synced before the checkpoint advances. After an
interrupted append, startup truncates uncommitted CSV tails and rebuilds the
summary CSVs from committed request rows. On ordinary rename-based logrotate,
it drains the old file and retained uncompressed numeric rotations (`access.log.1`,
`access.log.2`, …) before following the new active log. Keep the old rotated
files until they have been read; compression or deletion before service catch-up
causes a safe error and systemd restart instead of silent skipping. Copytruncate
is detected if the new size is below the saved offset, though a truncate-and-
regrow race between polls can still lose records. Rename/create rotation is
preferred. The service does not change Nginx configuration or install itself
into systemd's enabled services.

The installer creates `/etc/subsite-monitor/monitor.conf`, owned by root and
mode 0600, within a mode 0700 directory. Edit it with `sudoedit`. The default
`access_log` is `/var/log/subsite-monitor/access.log`. The no-argument
UI and `ui` command read this config, so use `sudo subsite-monitor` or
`sudo subsite-monitor ui`. `SUBSITE_MONITOR_CONFIG` selects another root-only
config, and `SUBSITE_MONITOR_LOG` overrides only the log path. `ui --log
<path>` overrides the config and can be used by a non-root user with read
permission.

The UI reads `server_name` directives from the Nginx main config and included
files, then combines those names with hostnames in the log. If the configured
log cannot be opened, it still displays the configured site inventory with an
on-screen warning; request activity is unavailable until the path is corrected. Literal and
wildcard names are listed; regex server names and `_` are skipped. The log
must use the host-aware format below to attribute activity by site. Nginx's
standard access log usually omits the hostname, so use the documented custom
format for per-site request counts. The overview defaults to latest access
first. Press `1` for website, `2` for requests, or `3` for last access; press
the active number again to reverse sort direction. The active heading displays
`v` for descending or `^` for ascending. The overview is a snapshot. Press
up/down or `j`/`k` to select a website;
Enter opens every stored request for that site. Each request occupies one table row with server-local timestamp and numeric
offset, client IP, method/status, and request URL columns. The view opens on the
newest 100 requests, newest first. Down moves toward older requests; Page
Down loads the next older page. Press `1` for time, `2` for client IP, `3` for
method/status, or `4` for URL to sort the current page; press the same number
again to reverse its order. The active header shows `v` for descending or `^`
for ascending. Up/Page Up move toward newer requests, and left/right scroll the
selected URL. Sorting is limited to the current page so memory remains bounded
for large logs. Column headers use descriptive names; the sort keys remain
shown in the overview and request-history footers.
Escape returns to the overview; `r` refreshes the log and `q` exits. It requires
an interactive terminal. Footer actions use the `Action: key | Action: key`
style, grouped by purpose and separated with vertical dividers. Unselected rows
use normal terminal brightness and the
selected row is bold white on blue. The UI intentionally shows full query
strings and IPs.

In the site overview, press `u` to check for newer releases that have the
supported x86_64 Linux archive and a GitHub SHA-256 digest. Select a version to
read its release notes, then press Enter and `y` to confirm installation. The
UI invokes the updater helper at `/usr/share/subsite-monitor/installer.sh`;
when not already root it asks `sudo` to run that helper. The installer verifies
the downloaded archive digest before replacing the executable, and may reload
Nginx and restart the monitor service. A successful update leaves the current
UI process running its old in-memory version; quit and launch it again to use
the new binary. Press `q` or Escape to leave the release list without changing
the installation. If the helper is missing, rerun the release installer to
install it.

The config accepts `access_log = /absolute/path` and
`nginx_config = /absolute/path` entries, ignores blank and comment lines, and
rejects unknown/duplicate keys, relative paths, control
characters, lines over 4,096 bytes, and files over 16 KiB. Reading the default
config requires effective root privileges. The program also verifies root
ownership and restrictive permissions before parsing it.

## Supported Nginx access-log format

The installer places `/etc/nginx/conf.d/subsite-monitor.conf`. If `nginx.conf`
includes `conf.d/*.conf` inside `http { }`, this snippet loads automatically.
The snippet defines `log_format subsite_monitor`
and writes to `/var/log/subsite-monitor/access.log`; the installer creates
that directory privately. `/var/log/subsite-monitor` keeps this application’s
log separate from Nginx's own logs under `/var/log/nginx`. The installer also
adds `/etc/logrotate.d/subsite-monitor`, which rotates this file daily, keeps
14 rotations, delays compression for one rotation, and asks the Nginx systemd
service to reopen its logs.

The release installer does not edit the main Nginx configuration. It runs
`nginx -t`, checks `nginx -T` to confirm the snippet was loaded, and reloads
`nginx.service` if systemd reports that it is active. If Nginx is missing or
inactive, the installer reports that it skipped validation/reload as
applicable; it does not start Nginx. Once the configured access log is
readable, the installer reloads systemd and enables/starts the monitor service.
On reinstallation, it restarts an already active monitor so it uses the
installed binary. If the log is unavailable, it skips automatic activation and
reports why; it does not stop a monitor that is already running. Confirm the
include is in the `http { }` context with `sudo nginx -T`. A server block with
its own `access_log`
directive overrides the shared log, so update that block to use the
`subsite_monitor` format if you need its requests.

```sh
sudo nginx -T
```

The app does not modify existing virtual-host files. The monitor log is a new,
separate file; existing Nginx logs remain untouched.

The installed snippet contains these directives (the access-log path is
customized if `SUBSITE_MONITOR_ACCESS_LOG` is set during installation):

```nginx
log_format subsite_monitor '$time_iso8601 $host $remote_addr "$request" $status';
access_log /var/log/subsite-monitor/access.log subsite_monitor;
```

The format is one line per request, in this exact field order:

```text
ISO-8601 timestamp host client-IP "METHOD request-target HTTP-version" status
```

For example:

```text
2026-10-27T14:45:32+00:00 live.example.test 203.0.113.10 "GET / HTTP/1.1" 200
```

The parser accepts timestamps with a numeric `+HH:MM` or `-HH:MM` offset,
hostname characters `A-Z`, `a-z`, digits, dot, hyphen, and underscore, and
IPv4/IPv6-style client address characters. Methods are limited to 32 bytes and
request targets to 8,192 bytes. The full input line is limited to 16,384 bytes.
Nginx's default log escaping for quotes, backslashes, and control bytes in
`$request` is recognized. Other log formats are not auto-detected.

When Nginx is behind a reverse proxy, `$remote_addr` will normally be the proxy
address. Configure Nginx's real IP handling only for known, trusted proxy
addresses if the actual client address is required; do not trust arbitrary
client-supplied forwarded headers.

## Pre-release installation test

For a server test before publishing a GitHub release, copy the generated
`installer.sh` and its matching archive from `packaging/dist/` to the server.
The generated installer accepts the local archive path while still checking
its pinned SHA-256:

```sh
sudo env SUBSITE_MONITOR_ARCHIVE=/tmp/subsite-monitor-0.1.0-alpha.6-linux-x86_64-gnu.tar.gz \
  ./installer.sh
```

The installer preserves an existing monitor config, Nginx snippet, and
logrotate policy. It migrates the previous default log path to
`/var/log/subsite-monitor/access.log`, but preserves custom paths. It does not
edit the main Nginx configuration. When Nginx is installed, the installer
tests its configuration, confirms the snippet is included, and reloads
`nginx.service` if active. When the configured log is readable, it enables
and starts the monitor service; on upgrades it restarts an active monitor. It
leaves an inactive Nginx service stopped. Check the installation output and
inspect the loaded config and service:

```sh
sudo nginx -T
sudo logrotate -d /etc/logrotate.d/subsite-monitor
sudo systemctl status subsite-monitor.service
```

The installer does not import historical log data; it starts collection from
the current end of the log.

Without options, an interactive installer lists up to 10 compatible published
releases; press Enter to accept the newest or enter another displayed number.
Use `./installer.sh --list` to inspect the options without installing,
`sudo ./installer.sh --latest` for a non-interactive latest install, or
`sudo ./installer.sh --version 0.1.0-alpha.5` to select a specific version.
Only releases with the x86_64 archive, `SHA256SUMS`, and valid SHA-256 asset
metadata are offered. Remote selection uses Python 3 and requires `curl` or
`wget` for downloads.

## Current limits

Manual `collect` remains a full snapshot operation and replaces files for dates
present in that scan. Continuous service collection is checkpointed, but
copytruncate has an unavoidable race if the file is truncated and regrows past
the saved offset between polls. Only uncompressed numeric logrotate names are
supported for catch-up after rename rotation. See the project
[TODO](../TODO.md) for remaining work.

Last reviewed: 2026-10-29. This manual describes the current
CLI and TUI.
