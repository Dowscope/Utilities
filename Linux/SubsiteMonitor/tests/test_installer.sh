#!/bin/sh
set -eu

script_dir=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
asset=${1:?usage: test_installer.sh <release-archive>}
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
cp "$TEST_RELEASE_ARCHIVE" "$output"
MOCK_CURL
chmod 0755 "$tmp_dir/mock-bin/curl"

PATH="$tmp_dir/mock-bin:$PATH" \
TEST_RELEASE_ARCHIVE="$asset" \
SUBSITE_MONITOR_INSTALL_DIR="$tmp_dir/install" \
SUBSITE_MONITOR_CONFIG="$tmp_dir/etc/monitor.conf" \
    sh "$script_dir/installer.sh"
[ "$("$tmp_dir/install/subsite-monitor" --version)" = \
  'subsite-monitor 0.1.0-alpha.3' ]
[ "$(stat -c '%a' "$tmp_dir/etc/monitor.conf")" = 600 ]
grep -q '^access_log = /var/log/nginx/subsite-monitor-access.log$' \
    "$tmp_dir/etc/monitor.conf"

printf 'access_log = /srv/nginx/custom-access.log\n' \
    >"$tmp_dir/etc/monitor.conf"
chmod 0644 "$tmp_dir/etc/monitor.conf"
PATH="$tmp_dir/mock-bin:$PATH" \
TEST_RELEASE_ARCHIVE="$asset" \
SUBSITE_MONITOR_INSTALL_DIR="$tmp_dir/install" \
SUBSITE_MONITOR_CONFIG="$tmp_dir/etc/monitor.conf" \
    sh "$script_dir/installer.sh"
[ "$(stat -c '%a' "$tmp_dir/etc/monitor.conf")" = 600 ]
grep -q '^access_log = /srv/nginx/custom-access.log$' \
    "$tmp_dir/etc/monitor.conf"

printf 'not the release archive\n' >"$tmp_dir/corrupted.tar.gz"
if PATH="$tmp_dir/mock-bin:$PATH" \
    TEST_RELEASE_ARCHIVE="$tmp_dir/corrupted.tar.gz" \
    SUBSITE_MONITOR_INSTALL_DIR="$tmp_dir/should-not-install" \
    SUBSITE_MONITOR_CONFIG="$tmp_dir/should-not-config/monitor.conf" \
    sh "$script_dir/installer.sh" >"$tmp_dir/out" 2>"$tmp_dir/error"; then
    printf '%s\n' 'installer accepted a corrupted release archive' >&2
    exit 1
fi
grep -q 'checksum verification failed' "$tmp_dir/error"
[ ! -e "$tmp_dir/should-not-install/subsite-monitor" ]
[ ! -e "$tmp_dir/should-not-config/monitor.conf" ]

printf '%s\n' 'Installer tests passed'
