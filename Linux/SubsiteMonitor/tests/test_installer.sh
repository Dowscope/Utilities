#!/bin/sh
set -eu

script_dir=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
asset=${1:?usage: test_installer.sh <release-archive>}
installer=${2:-$script_dir/installer.sh}
expected_version=$(sed -n "s/^APP_VERSION='\([^']*\)'$/\1/p" "$installer")
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

cat >"$tmp_dir/mock-bin/nginx" <<'MOCK_NGINX'
#!/bin/sh
set -eu
printf '%s\n' "$*" >>"${TEST_NGINX_CALLS:-/dev/null}"
case "$1" in
    -t) exit 0 ;;
    -T)
        printf '# configuration file %s/subsite-monitor.conf:\n' \
            "$SUBSITE_MONITOR_NGINX_CONF_DIR"
        ;;
    *) exit 2 ;;
esac
MOCK_NGINX
chmod 0755 "$tmp_dir/mock-bin/nginx"

cat >"$tmp_dir/mock-bin/systemctl" <<'MOCK_SYSTEMCTL'
#!/bin/sh
set -eu
printf '%s\n' "$*" >>"${TEST_SYSTEMCTL_CALLS:-/dev/null}"
case "$1" in
    is-active)
        case "$3" in
            nginx.service)
                [ "${TEST_SYSTEMCTL_NGINX_INACTIVE:-0}" = 0 ]
                ;;
            subsite-monitor.service)
                [ "${TEST_SYSTEMCTL_MONITOR_ACTIVE:-0}" = 1 ]
                ;;
            *) exit 2 ;;
        esac
        ;;
    reload)
        if [ "$2" = nginx.service ]; then
            mkdir -p "$(dirname "$TEST_ACCESS_LOG")"
            : >"$TEST_ACCESS_LOG"
        fi
        ;;
    daemon-reload) ;;
    enable|restart) ;;
    *) exit 2 ;;
esac
MOCK_SYSTEMCTL
chmod 0755 "$tmp_dir/mock-bin/systemctl"

printf 'ID=debian\nVERSION="12 (bookworm)"\n' >"$tmp_dir/os-release"
sed "s|^\\. /etc/os-release$|. $tmp_dir/os-release|" "$installer" \
    >"$tmp_dir/installer-debian"
chmod 0755 "$tmp_dir/installer-debian"

PATH="$tmp_dir/mock-bin:$PATH" \
TEST_RELEASE_ARCHIVE="$asset" \
TEST_CURL_SHOULD_FAIL=1 \
TEST_NGINX_CALLS="$tmp_dir/nginx-calls" \
TEST_SYSTEMCTL_CALLS="$tmp_dir/systemctl-calls" \
TEST_ACCESS_LOG="$tmp_dir/var/log/subsite-monitor/access.log" \
SUBSITE_MONITOR_ARCHIVE="$asset" \
SUBSITE_MONITOR_INSTALL_DIR="$tmp_dir/install" \
SUBSITE_MONITOR_CONFIG="$tmp_dir/etc/monitor.conf" \
SUBSITE_MONITOR_SYSTEMD_UNIT_DIR="$tmp_dir/systemd" \
SUBSITE_MONITOR_STATE_DIR="$tmp_dir/state" \
SUBSITE_MONITOR_NGINX_CONF_DIR="$tmp_dir/nginx/conf.d" \
SUBSITE_MONITOR_LOGROTATE_FILE="$tmp_dir/logrotate.d/subsite-monitor" \
SUBSITE_MONITOR_ACCESS_LOG="$tmp_dir/var/log/subsite-monitor/access.log" \
    sh "$tmp_dir/installer-debian"
[ "$("$tmp_dir/install/subsite-monitor" --version)" = \
  "subsite-monitor $expected_version" ]
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
grep -qx 'reload nginx.service' "$tmp_dir/systemctl-calls"
grep -qx 'daemon-reload' "$tmp_dir/systemctl-calls"
grep -qx 'enable --now subsite-monitor.service' "$tmp_dir/systemctl-calls"
grep -qx -- '-t' "$tmp_dir/nginx-calls"
grep -qx -- '-T' "$tmp_dir/nginx-calls"
printf '# keep administrator edits\n' \
    >>"$tmp_dir/nginx/conf.d/subsite-monitor.conf"
printf '# keep administrator rotation edits\n' \
    >>"$tmp_dir/logrotate.d/subsite-monitor"

printf 'access_log = %s/custom/custom-access.log\n' "$tmp_dir" \
    >"$tmp_dir/etc/monitor.conf"
chmod 0644 "$tmp_dir/etc/monitor.conf"
mkdir -p "$tmp_dir/custom"
: >"$tmp_dir/custom/custom-access.log"
sed -i "s|^access_log .* subsite_monitor;\$|access_log $tmp_dir/custom/custom-access.log subsite_monitor;|" \
    "$tmp_dir/nginx/conf.d/subsite-monitor.conf"
sed -i "1s|^.* {|$tmp_dir/custom/custom-access.log {|" \
    "$tmp_dir/logrotate.d/subsite-monitor"
PATH="$tmp_dir/mock-bin:$PATH" \
TEST_RELEASE_ARCHIVE="$asset" \
TEST_SYSTEMCTL_NGINX_INACTIVE=1 \
TEST_SYSTEMCTL_MONITOR_ACTIVE=1 \
TEST_SYSTEMCTL_CALLS="$tmp_dir/systemctl-calls" \
TEST_ACCESS_LOG="$tmp_dir/custom/custom-access.log" \
SUBSITE_MONITOR_ARCHIVE= \
SUBSITE_MONITOR_INSTALL_DIR="$tmp_dir/install" \
SUBSITE_MONITOR_CONFIG="$tmp_dir/etc/monitor.conf" \
SUBSITE_MONITOR_SYSTEMD_UNIT_DIR="$tmp_dir/systemd" \
SUBSITE_MONITOR_STATE_DIR="$tmp_dir/state" \
SUBSITE_MONITOR_NGINX_CONF_DIR="$tmp_dir/nginx/conf.d" \
SUBSITE_MONITOR_LOGROTATE_FILE="$tmp_dir/logrotate.d/subsite-monitor" \
SUBSITE_MONITOR_ACCESS_LOG="$tmp_dir/var/log/subsite-monitor/access.log" \
    sh "$tmp_dir/installer-debian"
[ "$(stat -c '%a' "$tmp_dir/etc/monitor.conf")" = 600 ]
grep -Fq "access_log = $tmp_dir/custom/custom-access.log" \
    "$tmp_dir/etc/monitor.conf"
grep -q '^# keep administrator edits$' \
    "$tmp_dir/nginx/conf.d/subsite-monitor.conf"
grep -q '^# keep administrator rotation edits$' \
    "$tmp_dir/logrotate.d/subsite-monitor"
test "$(grep -c '^reload nginx.service$' "$tmp_dir/systemctl-calls")" = 1
grep -qx 'enable subsite-monitor.service' "$tmp_dir/systemctl-calls"
grep -qx 'restart subsite-monitor.service' "$tmp_dir/systemctl-calls"
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
TEST_SYSTEMCTL_CALLS="$tmp_dir/systemctl-calls" \
TEST_ACCESS_LOG="$tmp_dir/legacy-var/log/subsite-monitor/access.log" \
    sh "$tmp_dir/installer-debian"
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
