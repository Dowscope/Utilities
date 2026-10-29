#!/bin/sh
set -eu

APP_VERSION='0.1.0-alpha.12'
ASSET='subsite-monitor-0.1.0-alpha.12-linux-x86_64-gnu.tar.gz'
ASSET_SHA256='8ba1e0bf110a6d689bea3901a879575182667c660580cd350f6583f65d955ffc'
RELEASE_API_URL='https://api.github.com/repos/Dowscope/Utilities/releases?per_page=100'
RELEASE_MODE='auto'
REQUESTED_VERSION=''
INSTALL_DIR=${SUBSITE_MONITOR_INSTALL_DIR:-/usr/local/bin}
CONFIG_FILE=${SUBSITE_MONITOR_CONFIG:-/etc/subsite-monitor/monitor.conf}
SYSTEMD_UNIT_DIR=${SUBSITE_MONITOR_SYSTEMD_UNIT_DIR:-/usr/lib/systemd/system}
STATE_DIR=${SUBSITE_MONITOR_STATE_DIR:-/var/lib/subsite-monitor}
NGINX_CONF_DIR=${SUBSITE_MONITOR_NGINX_CONF_DIR:-/etc/nginx/conf.d}
LOGROTATE_FILE=${SUBSITE_MONITOR_LOGROTATE_FILE:-/etc/logrotate.d/subsite-monitor}
UPDATER_DIR=${SUBSITE_MONITOR_UPDATER_DIR:-/usr/share/subsite-monitor}
ACCESS_LOG=${SUBSITE_MONITOR_ACCESS_LOG:-/var/log/subsite-monitor/access.log}

fail() {
    printf 'subsite-monitor installer: %s\n' "$1" >&2
    exit 1
}

case "$#" in
    0) ;;
    1)
        case "$1" in
            --latest) RELEASE_MODE='latest' ;;
            --list) RELEASE_MODE='list' ;;
            --help|-h)
                cat <<'HELP'
Usage: installer.sh [--latest | --list | --version <version>]

Without options, interactively choose from up to 10 available releases. In a
non-interactive shell, the newest available release is selected automatically.
HELP
                exit 0
                ;;
            *) fail "unknown option: $1" ;;
        esac
        ;;
    2)
        if [ "$1" = '--version' ]; then
            RELEASE_MODE='version'
            REQUESTED_VERSION=$2
        else
            fail 'usage: installer.sh [--latest | --list | --version <version>]'
        fi
        ;;
    *) fail 'usage: installer.sh [--latest | --list | --version <version>]' ;;
esac

case "$(uname -s)" in
    Linux) ;;
    *) fail 'this alpha supports Linux only' ;;
esac

case "$(uname -m)" in
    x86_64|amd64) ;;
    *) fail 'this alpha release supports x86_64 only' ;;
esac

if [ ! -r /etc/os-release ]; then
    fail 'cannot identify this Linux distribution'
fi
# os-release is the operating system's standard, administrator-controlled file.
. /etc/os-release
case "${ID:-}" in
    debian|ubuntu|arch) ;;
    *) fail "unsupported distribution: ${ID:-unknown}; supported: Debian, Ubuntu, Arch" ;;
esac

if ! command -v sha256sum >/dev/null 2>&1; then
    fail 'sha256sum is required to verify the release archive'
fi
if ! command -v tar >/dev/null 2>&1; then
    fail 'tar is required to extract the release archive'
fi
if { [ -z "${SUBSITE_MONITOR_ARCHIVE:-}" ] || [ "$RELEASE_MODE" = 'list' ]; } &&
    ! command -v python3 >/dev/null 2>&1; then
    fail 'python3 is required to read the GitHub release list safely'
fi
if { [ -z "${SUBSITE_MONITOR_ARCHIVE:-}" ] || [ "$RELEASE_MODE" = 'list' ]; } &&
    ! command -v curl >/dev/null 2>&1 && ! command -v wget >/dev/null 2>&1; then
    fail 'curl or wget is required to download the release'
fi

work_dir=$(mktemp -d "${TMPDIR:-/tmp}/subsite-monitor.XXXXXX") ||
    fail 'could not create a temporary directory'
staged_binary=''
staged_config=''
staged_updater=''
cleanup() {
    rm -rf "$work_dir"
    if [ -n "$staged_binary" ]; then
        rm -f "$staged_binary"
    fi
    if [ -n "$staged_config" ]; then
        rm -f "$staged_config"
    fi
    if [ -n "$staged_updater" ]; then
        rm -f "$staged_updater"
    fi
}
trap cleanup EXIT
trap 'exit 1' HUP INT TERM
cp "$0" "$work_dir/installer.sh" || fail 'could not stage the updater installer'

download_file() {
    output_path=$1
    source_url=$2
    if command -v curl >/dev/null 2>&1; then
        curl --fail --location --silent --show-error \
            --output "$output_path" "$source_url"
    else
        wget --quiet --output-document="$output_path" "$source_url"
    fi
}

select_release() {
    catalog_json="$work_dir/releases.json"
    release_list="$work_dir/available-releases.txt"
    download_file "$catalog_json" "$RELEASE_API_URL" ||
        fail 'could not retrieve the GitHub release list'

    if ! python3 - "$catalog_json" >"$release_list" <<'PYTHON'
import json
import re
import sys

version_pattern = re.compile(r"[0-9]+\.[0-9]+\.[0-9]+(?:-[0-9A-Za-z.-]+)?\Z")
digest_pattern = re.compile(r"sha256:([0-9a-fA-F]{64})\Z")

def version_key(version):
    core, separator, prerelease = version.partition("-")
    core_key = tuple(int(part) for part in core.split("."))
    if not separator:
        return core_key, 1, ()
    prerelease_key = tuple(
        (0, int(part)) if part.isdigit() else (1, part)
        for part in prerelease.split(".")
    )
    return core_key, 0, prerelease_key

try:
    with open(sys.argv[1], encoding="utf-8") as source:
        releases = json.load(source)
except (OSError, json.JSONDecodeError) as error:
    print(f"cannot parse GitHub release data: {error}", file=sys.stderr)
    sys.exit(1)

if not isinstance(releases, list):
    print("GitHub returned an unexpected release-list response", file=sys.stderr)
    sys.exit(1)

available = []
for release in releases:
    if not isinstance(release, dict) or release.get("draft") is True:
        continue
    tag = release.get("tag_name", "")
    version = tag[1:] if tag.startswith("v") else tag
    if not version_pattern.fullmatch(version):
        continue
    assets = release.get("assets", [])
    if not isinstance(assets, list):
        continue
    expected_archive = f"subsite-monitor-{version}-linux-x86_64-gnu.tar.gz"
    archive = next((item for item in assets
                   if isinstance(item, dict) and item.get("name") == expected_archive), None)
    checksums = any(isinstance(item, dict) and item.get("name") == "SHA256SUMS"
                    for item in assets)
    archive_digest = archive.get("digest") if archive else None
    digest = digest_pattern.fullmatch(archive_digest) \
        if isinstance(archive_digest, str) else None
    if not checksums or digest is None:
        continue
    published = release.get("published_at", "")
    date = published[:10] if isinstance(published, str) else ""
    available.append((version, tag, digest.group(1), date))
available.sort(key=lambda release: version_key(release[0]), reverse=True)
available = available[:10]

for version, tag, digest, date in available:
    print(f"{version}\t{tag}\t{digest}\t{date}")
PYTHON
    then
        fail 'invalid GitHub release-list response'
    fi
    [ -s "$release_list" ] || fail 'no compatible releases with verified assets were found'

    if [ "$RELEASE_MODE" = 'list' ]; then
        printf '%s\n' 'Available Subsite Monitor releases (x86_64 Linux):'
        awk -F '\t' '{ printf "  %2d) %-22s %s\n", NR, $1, $4 }' "$release_list"
        exit 0
    fi

    if [ "$RELEASE_MODE" = 'version' ]; then
        selected_line=$(awk -F '\t' -v wanted="$REQUESTED_VERSION" \
            '$1 == wanted || $2 == wanted { print; found=1 } END { exit !found }' \
            "$release_list") ||
            fail "version is not among the 10 newest available releases: $REQUESTED_VERSION"
    elif [ "$RELEASE_MODE" = 'auto' ] && [ -t 0 ] && [ -t 1 ]; then
        printf '%s\n' 'Available Subsite Monitor releases (x86_64 Linux):'
        awk -F '\t' '{ printf "  %2d) %-22s %s\n", NR, $1, $4 }' "$release_list"
        printf 'Select a version [1]: '
        IFS= read -r selection || selection=''
        case "$selection" in
            '') selection=1 ;;
            0) fail 'release selection is out of range' ;;
            *[!0-9]*) fail 'selection must be a release number' ;;
        esac
        selected_line=$(sed -n "${selection}p" "$release_list")
        [ -n "$selected_line" ] || fail 'release selection is out of range'
    else
        selected_line=$(sed -n '1p' "$release_list")
    fi

    IFS="$(printf '\t')" read -r APP_VERSION RELEASE_TAG ASSET_SHA256 RELEASE_DATE <<EOF
$selected_line
EOF
    ASSET="subsite-monitor-$APP_VERSION-linux-x86_64-gnu.tar.gz"
    ASSET_URL="https://github.com/Dowscope/Utilities/releases/download/$RELEASE_TAG/$ASSET"
    printf 'Selected Subsite Monitor %s (%s).\n' "$APP_VERSION" "$RELEASE_DATE"
}

if [ -z "${SUBSITE_MONITOR_ARCHIVE:-}" ] || [ "$RELEASE_MODE" = 'list' ]; then
    select_release
fi

archive="$work_dir/$ASSET"
if [ -n "${SUBSITE_MONITOR_ARCHIVE:-}" ]; then
    if [ -L "$SUBSITE_MONITOR_ARCHIVE" ] ||
        [ ! -f "$SUBSITE_MONITOR_ARCHIVE" ]; then
        fail 'local release archive must be a regular, non-symlink file'
    fi
    cp "$SUBSITE_MONITOR_ARCHIVE" "$archive" ||
        fail 'cannot copy local release archive'
    [ "$RELEASE_MODE" != 'version' ] ||
        [ "$REQUESTED_VERSION" = "$APP_VERSION" ] ||
        fail 'local archive version does not match the requested version'
else
    download_file "$archive" "$ASSET_URL" || fail 'release download failed'
fi

printf '%s  %s\n' "$ASSET_SHA256" "$archive" | sha256sum --check --status - ||
    fail 'release archive checksum verification failed'

mkdir -p "$work_dir/extracted"
tar -xzf "$archive" -C "$work_dir/extracted" \
    bin/subsite-monitor config/monitor.conf.example \
    config/nginx-log-format.conf.example \
    config/subsite-monitor.logrotate \
    systemd/subsite-monitor.service ||
    fail 'release archive could not be extracted'
binary="$work_dir/extracted/bin/subsite-monitor"
if [ ! -f "$binary" ] || [ ! -x "$binary" ]; then
    fail 'release archive does not contain the expected executable'
fi
config_template="$work_dir/extracted/config/monitor.conf.example"
nginx_format_template="$work_dir/extracted/config/nginx-log-format.conf.example"
logrotate_template="$work_dir/extracted/config/subsite-monitor.logrotate"
service_template="$work_dir/extracted/systemd/subsite-monitor.service"
if [ ! -f "$config_template" ]; then
    fail 'release archive does not contain the default configuration'
fi
if [ ! -f "$service_template" ]; then
    fail 'release archive does not contain the systemd unit'
fi
if [ ! -f "$nginx_format_template" ]; then
    fail 'release archive does not contain the Nginx log format example'
fi
if [ ! -f "$logrotate_template" ]; then
    fail 'release archive does not contain the logrotate policy'
fi
if [ "$("$binary" --version)" != "subsite-monitor $APP_VERSION" ]; then
    fail 'downloaded executable version does not match the release'
fi
for configured_path in "$ACCESS_LOG" "$NGINX_CONF_DIR" "$LOGROTATE_FILE"; do
    case "$configured_path" in
        /*) ;;
        *) fail "configured path must be absolute: $configured_path" ;;
    esac
    case "$configured_path" in
        *[!A-Za-z0-9_./-]*)
            fail "configured path contains unsupported characters: $configured_path"
            ;;
    esac
    case "$configured_path" in
        */../*|*/..|*/./*|*/.)
            fail "configured path contains a dot segment: $configured_path"
            ;;
    esac
done
case "$UPDATER_DIR" in
    /*) ;;
    *) fail 'updater directory must be absolute' ;;
esac
case "$UPDATER_DIR" in
    *[!A-Za-z0-9_./-]*) fail 'updater directory contains unsupported characters' ;;
esac
case "$UPDATER_DIR" in
    */../*|*/..|*/./*|*/.) fail 'updater directory contains a dot segment' ;;
esac
case "$INSTALL_DIR" in
    /*) ;;
    *) fail 'installation directory must be absolute' ;;
esac
case "$INSTALL_DIR" in
    *[!A-Za-z0-9_./-]*)
        fail 'installation directory contains unsupported characters for systemd'
        ;;
esac
service_file="$work_dir/subsite-monitor.service"
sed "s|^ExecStart=/usr/local/bin/subsite-monitor service$|ExecStart=$INSTALL_DIR/subsite-monitor service|" \
    "$service_template" >"$service_file" ||
    fail 'could not prepare systemd unit'
monitor_config_file="$work_dir/monitor.conf"
sed "s|^access_log = .*|access_log = $ACCESS_LOG|" \
    "$config_template" >"$monitor_config_file" ||
    fail 'could not prepare monitor configuration'

case "$CONFIG_FILE" in
    /*/*) ;;
    *) fail 'configuration path must be absolute' ;;
esac
config_dir=${CONFIG_FILE%/*}
install -d -m 0700 "$config_dir" ||
    fail "cannot create configuration directory: $config_dir"
if [ "$(id -u)" -eq 0 ]; then
    chown root:root "$config_dir" ||
        fail "cannot set configuration directory ownership: $config_dir"
    chmod 0700 "$config_dir" ||
        fail "cannot secure configuration directory: $config_dir"
fi
if [ -L "$CONFIG_FILE" ]; then
    fail 'configuration file must not be a symbolic link'
elif [ -e "$CONFIG_FILE" ]; then
    if [ ! -f "$CONFIG_FILE" ]; then
        fail 'existing configuration path is not a regular file'
    fi
    chmod 0600 "$CONFIG_FILE" ||
        fail "cannot secure configuration file: $CONFIG_FILE"
    if [ "$(id -u)" -eq 0 ]; then
        chown root:root "$CONFIG_FILE" ||
            fail "cannot set configuration ownership: $CONFIG_FILE"
    fi
    if grep -Fqx 'access_log = /var/log/nginx/subsite-monitor-access.log' \
        "$CONFIG_FILE"; then
        staged_config=$(mktemp "$config_dir/.monitor.conf.XXXXXX") ||
            fail 'cannot stage monitor config migration'
        sed "s|^access_log = /var/log/nginx/subsite-monitor-access.log$|access_log = $ACCESS_LOG|" \
            "$CONFIG_FILE" >"$staged_config" ||
            fail 'cannot update the legacy default access-log path'
        chmod 0600 "$staged_config" ||
            fail 'cannot secure migrated monitor config'
        if [ "$(id -u)" -eq 0 ]; then
            chown root:root "$staged_config" ||
                fail 'cannot set migrated config ownership'
        fi
        mv -f "$staged_config" "$CONFIG_FILE" ||
            fail 'cannot activate migrated monitor config'
        staged_config=''
    fi
else
    install -m 0600 "$monitor_config_file" "$CONFIG_FILE" ||
        fail "cannot install configuration file: $CONFIG_FILE"
fi

configured_access_log=$(sed -n 's/^access_log = //p' "$CONFIG_FILE")
if [ -z "$configured_access_log" ]; then
    fail "monitor config has no access_log value: $CONFIG_FILE"
fi
ACCESS_LOG=$configured_access_log
case "$ACCESS_LOG" in
    /*) ;;
    *) fail "configured access log must be absolute: $ACCESS_LOG" ;;
esac
case "$ACCESS_LOG" in
    *[!A-Za-z0-9_./-]*)
        fail "configured access-log path contains unsupported characters: $ACCESS_LOG"
        ;;
esac
case "$ACCESS_LOG" in
    */../*|*/..|*/./*|*/.)
        fail "configured access-log path contains a dot segment: $ACCESS_LOG"
        ;;
esac
nginx_config_file="$work_dir/subsite-monitor.conf"
sed "s|^access_log /var/log/subsite-monitor/access.log subsite_monitor;$|access_log $ACCESS_LOG subsite_monitor;|" \
    "$nginx_format_template" >"$nginx_config_file" ||
    fail 'could not prepare Nginx configuration snippet'
logrotate_config_file="$work_dir/subsite-monitor.logrotate"
sed "s|^/var/log/subsite-monitor/access.log {|$ACCESS_LOG {|" \
    "$logrotate_template" >"$logrotate_config_file" ||
    fail 'could not prepare logrotate configuration'

nginx_config_path="$NGINX_CONF_DIR/subsite-monitor.conf"
if [ -L "$NGINX_CONF_DIR" ]; then
    fail 'Nginx configuration directory must not be a symbolic link'
fi
install -d -m 0755 "$NGINX_CONF_DIR" ||
    fail "cannot create Nginx configuration directory: $NGINX_CONF_DIR"
if [ -L "$nginx_config_path" ]; then
    fail 'Nginx Subsite Monitor snippet must not be a symbolic link'
elif [ -e "$nginx_config_path" ]; then
    if [ ! -f "$nginx_config_path" ]; then
        fail 'existing Nginx snippet path is not a regular file'
    fi
    if ! grep -Fqx "access_log $ACCESS_LOG subsite_monitor;" \
        "$nginx_config_path"; then
        fail "existing Nginx snippet targets a different log: $nginx_config_path"
    fi
else
    install -m 0644 "$nginx_config_file" "$nginx_config_path" ||
        fail "cannot install Nginx snippet: $nginx_config_path"
fi

log_directory=${ACCESS_LOG%/*}
[ -n "$log_directory" ] || log_directory=/
if [ -L "$log_directory" ]; then
    fail 'access-log directory must not be a symbolic link'
fi
install -d -m 0750 "$log_directory" ||
    fail "cannot create access-log directory: $log_directory"
if [ "$(id -u)" -eq 0 ]; then
    chown root:root "$log_directory" ||
        fail "cannot set access-log directory ownership: $log_directory"
    chmod 0750 "$log_directory" ||
        fail "cannot secure access-log directory: $log_directory"
fi

logrotate_directory=${LOGROTATE_FILE%/*}
install -d -m 0755 "$logrotate_directory" ||
    fail "cannot create logrotate directory: $logrotate_directory"
if [ -L "$LOGROTATE_FILE" ]; then
    fail 'Subsite Monitor logrotate file must not be a symbolic link'
elif [ -e "$LOGROTATE_FILE" ]; then
    if [ ! -f "$LOGROTATE_FILE" ]; then
        fail 'existing logrotate path is not a regular file'
    fi
    if ! grep -Fqx "$ACCESS_LOG {" "$LOGROTATE_FILE"; then
        fail "existing logrotate policy targets a different log: $LOGROTATE_FILE"
    fi
else
    install -m 0644 "$logrotate_config_file" "$LOGROTATE_FILE" ||
        fail "cannot install logrotate policy: $LOGROTATE_FILE"
fi

install -d -m 0700 "$STATE_DIR" ||
    fail "cannot create state directory: $STATE_DIR"
if [ "$(id -u)" -eq 0 ]; then
    chown root:root "$STATE_DIR" ||
        fail "cannot set state directory ownership: $STATE_DIR"
    chmod 0700 "$STATE_DIR" ||
        fail "cannot secure state directory: $STATE_DIR"
fi

mkdir -p "$INSTALL_DIR" || fail "cannot create install directory: $INSTALL_DIR"
staged_binary=$(mktemp "$INSTALL_DIR/.subsite-monitor.XXXXXX") ||
    fail "cannot write to install directory: $INSTALL_DIR"
install -m 0755 "$binary" "$staged_binary" || fail 'could not stage executable'
mv -f "$staged_binary" "$INSTALL_DIR/subsite-monitor" ||
    fail 'could not install executable'
staged_binary=''

install -d -m 0755 "$SYSTEMD_UNIT_DIR" ||
    fail "cannot create systemd unit directory: $SYSTEMD_UNIT_DIR"
install -m 0644 "$service_file" \
    "$SYSTEMD_UNIT_DIR/subsite-monitor.service" ||
    fail 'could not install systemd unit'

if [ -L "$UPDATER_DIR" ]; then
    fail 'updater directory must not be a symbolic link'
fi
install -d -m 0755 "$UPDATER_DIR" ||
    fail "cannot create updater directory: $UPDATER_DIR"
updater_file="$UPDATER_DIR/installer.sh"
if [ -L "$updater_file" ]; then
    fail 'updater installer must not be a symbolic link'
fi
staged_updater=$(mktemp "$UPDATER_DIR/.installer.XXXXXX") ||
    fail "cannot stage updater installer in: $UPDATER_DIR"
install -m 0755 "$work_dir/installer.sh" "$staged_updater" ||
    fail 'could not stage updater installer'
mv -f "$staged_updater" "$updater_file" ||
    fail 'could not install updater installer'
staged_updater=''

if command -v nginx >/dev/null 2>&1; then
    nginx -t || fail 'Nginx configuration test failed; Nginx was not reloaded'
    nginx_dump="$work_dir/nginx-config.txt"
    nginx -T >"$nginx_dump" 2>&1 ||
        fail 'could not inspect the active Nginx configuration'
    if ! grep -Fq "# configuration file $nginx_config_path:" "$nginx_dump"; then
        fail "Nginx did not load $nginx_config_path; ensure conf.d/*.conf is included inside http {}"
    fi
    if command -v systemctl >/dev/null 2>&1 &&
        systemctl is-active --quiet nginx.service; then
        systemctl reload nginx.service || fail 'Nginx reload failed'
        printf '%s\n' 'Nginx configuration validated and nginx.service reloaded.'
    else
        printf '%s\n' 'Nginx configuration validated; nginx.service is not active, so it was not reloaded.'
    fi
else
    printf '%s\n' 'Nginx is not installed; configuration test and reload were skipped.'
fi

if command -v systemctl >/dev/null 2>&1; then
    systemctl daemon-reload || fail 'could not reload systemd unit definitions'
    if [ -r "$ACCESS_LOG" ]; then
        if systemctl is-active --quiet subsite-monitor.service; then
            systemctl enable subsite-monitor.service ||
                fail 'could not enable the Subsite Monitor service'
            systemctl restart subsite-monitor.service ||
                fail 'could not restart the Subsite Monitor service'
            printf '%s\n' 'Subsite Monitor service enabled and restarted.'
        else
            systemctl enable --now subsite-monitor.service ||
                fail 'could not enable and start the Subsite Monitor service'
            printf '%s\n' 'Subsite Monitor service enabled and started.'
        fi
    else
        printf 'Subsite Monitor service was not enabled or started because the access log is not readable: %s\n' \
            "$ACCESS_LOG"
    fi
else
    printf '%s\n' 'systemctl is unavailable; the Subsite Monitor service was not enabled or started.'
fi

printf 'Installed subsite-monitor %s to %s/subsite-monitor and %s/subsite-monitor.service\n' \
    "$APP_VERSION" "$INSTALL_DIR" "$SYSTEMD_UNIT_DIR"
printf 'Nginx snippet: %s\n' "$nginx_config_path"
printf 'Access log: %s\n' "$ACCESS_LOG"
printf 'Log rotation policy: %s\n' "$LOGROTATE_FILE"
printf 'Update helper: %s\n' "$updater_file"
printf '%s\n' 'The installer did not edit the main Nginx configuration.'
