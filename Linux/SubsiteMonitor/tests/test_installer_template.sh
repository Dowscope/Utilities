#!/bin/sh
set -eu

script_dir=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
program=${1:-$script_dir/subsite-monitor}
tmp_dir=$(mktemp -d)
trap 'rm -rf "$tmp_dir"' EXIT HUP INT TERM

version=$($program --version)
version=${version#subsite-monitor }
archive_name="subsite-monitor-$version-linux-x86_64-gnu.tar.gz"
mkdir -p "$tmp_dir/stage/bin" "$tmp_dir/stage/config" "$tmp_dir/stage/systemd"
install -m 0755 "$program" "$tmp_dir/stage/bin/subsite-monitor"
install -m 0644 "$script_dir/config/monitor.conf.example" \
    "$tmp_dir/stage/config/monitor.conf.example"
install -m 0644 "$script_dir/config/nginx-log-format.conf.example" \
    "$tmp_dir/stage/config/nginx-log-format.conf.example"
install -m 0644 "$script_dir/config/subsite-monitor.logrotate" \
    "$tmp_dir/stage/config/subsite-monitor.logrotate"
install -m 0644 "$script_dir/systemd/subsite-monitor.service" \
    "$tmp_dir/stage/systemd/subsite-monitor.service"
tar -czf "$tmp_dir/$archive_name" -C "$tmp_dir/stage" \
    bin/subsite-monitor config/monitor.conf.example \
    config/nginx-log-format.conf.example \
    config/subsite-monitor.logrotate \
    systemd/subsite-monitor.service
asset_hash=$(sha256sum "$tmp_dir/$archive_name" | awk '{print $1}')
sed -e "s/@VERSION@/$version/g" \
    -e "s/@ASSET_SHA256@/$asset_hash/g" \
    "$script_dir/packaging/installer.sh.in" >"$tmp_dir/installer.sh"
chmod 0755 "$tmp_dir/installer.sh"

sh "$script_dir/tests/test_installer.sh" \
    "$tmp_dir/$archive_name" "$tmp_dir/installer.sh"
echo 'Installer template tests passed'
