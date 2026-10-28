#!/bin/sh
set -eu

script_dir=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
asset=${1:?usage: test_installer.sh <release-archive>}
installer=${2:-$script_dir/installer.sh}
tmp_dir=$(mktemp -d)
trap 'rm -rf "$tmp_dir"' EXIT HUP INT TERM
mkdir -p "$tmp_dir/mock-bin"

cat >"$tmp_dir/mock-bin/curl" <<'MOCK_CURL'
#!/bin/sh
set -eu
output=''
while [ "$#" -gt 0 ]; do
    case "$1" in
        --output|-o)
            output=$2
            shift 2
            ;;
        *) shift ;;
    esac
done
[ -n "$output" ]
[ "${TEST_CURL_SHOULD_FAIL:-0}" != 1 ] || exit 9
cp "$TEST_RELEASE_ARCHIVE" "$output"
MOCK_CURL
chmod 0755 "$tmp_dir/mock-bin/curl"

PATH="$tmp_dir/mock-bin:$PATH" \
TEST_RELEASE_ARCHIVE="$asset" \
TEST_CURL_SHOULD_FAIL=1 \
SUBSITE_MONITOR_ARCHIVE="$asset" \
SUBSITE_MONITOR_INSTALL_DIR="$tmp_dir/install" \
SUBSITE_MONITOR_CONFIG="$tmp_dir/etc/monitor.conf" \
SUBSITE_MONITOR_SYSTEMD_UNIT_DIR="$tmp_dir/systemd" \
SUBSITE_MONITOR_STATE_DIR="$tmp_dir/state" \
SUBSITE_MONITOR_NGINX_CONF_DIR="$tmp_dir/nginx/conf.d" \
SUBSITE_MONITOR_LOGROTATE_FILE="$tmp_dir/logrotate.d/subsite-monitor" \
SUBSITE_MONITOR_ACCESS_LOG="$tmp_dir/var/log/subsite-monitor/access.log" \
    sh "$installer"
[ "$("$tmp_dir/install/subsite-monitor" --version)" = \
  'subsite-monitor 0.1.0-alpha.5' ]
[ "$(stat -c '%a' "$tmp_dir/etc/monitor.conf")" = 600 ]
grep -Fq "access_log = $tmp_dir/var/log/subsite-monitor/access.log" \
    "$tmp_dir/etc/monitor.conf"
grep -Fq "ExecStart=$tmp_dir/install/subsite-monitor service" \
    "$tmp_dir/systemd/subsite-monitor.service"
[ "$(stat -c '%a' "$tmp_dir/state")" = 700 ]
[ "$(stat -c '%a' "$tmp_dir/var/log/subsite-monitor")" = 750 ]
grep -q '^log_format subsite_monitor ' \
    "$tmp_dir/nginx/conf.d/subsite-monitor.conf"
grep -Fq "access_log $tmp_dir/var/log/subsite-monitor/access.log subsite_monitor;" \
    "$tmp_dir/nginx/conf.d/subsite-monitor.conf"
grep -Fq "$tmp_dir/var/log/subsite-monitor/access.log {" \
    "$tmp_dir/logrotate.d/subsite-monitor"
printf '# keep administrator edits\n' \
    >>"$tmp_dir/nginx/conf.d/subsite-monitor.conf"
printf '# keep administrator rotation edits\n' \
    >>"$tmp_dir/logrotate.d/subsite-monitor"

printf 'access_log = %s/custom/custom-access.log\n' "$tmp_dir" \
    >"$tmp_dir/etc/monitor.conf"
chmod 0644 "$tmp_dir/etc/monitor.conf"
sed -i "s|^access_log .* subsite_monitor;\$|access_log $tmp_dir/custom/custom-access.log subsite_monitor;|" \
    "$tmp_dir/nginx/conf.d/subsite-monitor.conf"
sed -i "1s|^.* {|$tmp_dir/custom/custom-access.log {|" \
    "$tmp_dir/logrotate.d/subsite-monitor"
PATH="$tmp_dir/mock-bin:$PATH" \
TEST_RELEASE_ARCHIVE="$asset" \
SUBSITE_MONITOR_ARCHIVE= \
SUBSITE_MONITOR_INSTALL_DIR="$tmp_dir/install" \
SUBSITE_MONITOR_CONFIG="$tmp_dir/etc/monitor.conf" \
SUBSITE_MONITOR_SYSTEMD_UNIT_DIR="$tmp_dir/systemd" \
SUBSITE_MONITOR_STATE_DIR="$tmp_dir/state" \
SUBSITE_MONITOR_NGINX_CONF_DIR="$tmp_dir/nginx/conf.d" \
SUBSITE_MONITOR_LOGROTATE_FILE="$tmp_dir/logrotate.d/subsite-monitor" \
SUBSITE_MONITOR_ACCESS_LOG="$tmp_dir/var/log/subsite-monitor/access.log" \
    sh "$installer"
[ "$(stat -c '%a' "$tmp_dir/etc/monitor.conf")" = 600 ]
grep -Fq "access_log = $tmp_dir/custom/custom-access.log" \
    "$tmp_dir/etc/monitor.conf"
grep -q '^# keep administrator edits$' \
    "$tmp_dir/nginx/conf.d/subsite-monitor.conf"
grep -q '^# keep administrator rotation edits$' \
    "$tmp_dir/logrotate.d/subsite-monitor"
grep -Fq "access_log $tmp_dir/custom/custom-access.log subsite_monitor;" \
    "$tmp_dir/nginx/conf.d/subsite-monitor.conf"
grep -Fq "$tmp_dir/custom/custom-access.log {" \
    "$tmp_dir/logrotate.d/subsite-monitor"

mkdir -p "$tmp_dir/legacy"
printf 'access_log = /var/log/nginx/subsite-monitor-access.log\nnginx_config = /etc/nginx/nginx.conf\n' \
    >"$tmp_dir/legacy/monitor.conf"
chmod 0644 "$tmp_dir/legacy/monitor.conf"
PATH="$tmp_dir/mock-bin:$PATH" \
TEST_RELEASE_ARCHIVE="$asset" \
SUBSITE_MONITOR_ARCHIVE="$asset" \
SUBSITE_MONITOR_INSTALL_DIR="$tmp_dir/legacy-bin" \
SUBSITE_MONITOR_CONFIG="$tmp_dir/legacy/monitor.conf" \
SUBSITE_MONITOR_SYSTEMD_UNIT_DIR="$tmp_dir/legacy-systemd" \
SUBSITE_MONITOR_STATE_DIR="$tmp_dir/legacy-state" \
SUBSITE_MONITOR_NGINX_CONF_DIR="$tmp_dir/legacy-nginx/conf.d" \
SUBSITE_MONITOR_LOGROTATE_FILE="$tmp_dir/legacy-logrotate/subsite-monitor" \
SUBSITE_MONITOR_ACCESS_LOG="$tmp_dir/legacy-var/log/subsite-monitor/access.log" \
    sh "$installer"
grep -Fq "access_log = $tmp_dir/legacy-var/log/subsite-monitor/access.log" \
    "$tmp_dir/legacy/monitor.conf"
grep -q '^nginx_config = /etc/nginx/nginx.conf$' \
    "$tmp_dir/legacy/monitor.conf"
[ "$(stat -c '%a' "$tmp_dir/legacy/monitor.conf")" = 600 ]
grep -Fq "access_log $tmp_dir/legacy-var/log/subsite-monitor/access.log subsite_monitor;" \
    "$tmp_dir/legacy-nginx/conf.d/subsite-monitor.conf"
grep -Fq "$tmp_dir/legacy-var/log/subsite-monitor/access.log {" \
    "$tmp_dir/legacy-logrotate/subsite-monitor"

printf 'not the release archive\n' >"$tmp_dir/corrupted.tar.gz"
if PATH="$tmp_dir/mock-bin:$PATH" \
    TEST_RELEASE_ARCHIVE="$tmp_dir/corrupted.tar.gz" \
    SUBSITE_MONITOR_INSTALL_DIR="$tmp_dir/should-not-install" \
    SUBSITE_MONITOR_CONFIG="$tmp_dir/should-not-config/monitor.conf" \
    sh "$installer" >"$tmp_dir/out" 2>"$tmp_dir/error"; then
    printf '%s\n' 'installer accepted a corrupted release archive' >&2
    exit 1
fi
grep -q 'checksum verification failed' "$tmp_dir/error"
[ ! -e "$tmp_dir/should-not-install/subsite-monitor" ]
[ ! -e "$tmp_dir/should-not-config/monitor.conf" ]

printf '%s\n' 'Installer tests passed'
