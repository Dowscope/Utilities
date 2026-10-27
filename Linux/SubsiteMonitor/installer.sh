#!/bin/sh
set -eu

VERSION='0.1.0-alpha.2'
ASSET='subsite-monitor-0.1.0-alpha.2-linux-x86_64-gnu.tar.gz'
ASSET_SHA256='2d3e312c48c58000e8cfc403ab355d1ca6ec2e5d95b034623e1858f82edb30b3'
ASSET_URL="https://github.com/Dowscope/Utilities/releases/download/v${VERSION}/${ASSET}"
INSTALL_DIR=${SUBSITE_MONITOR_INSTALL_DIR:-/usr/local/bin}
CONFIG_FILE=${SUBSITE_MONITOR_CONFIG:-/etc/subsite-monitor/monitor.conf}

fail() {
    printf 'subsite-monitor installer: %s\n' "$1" >&2
    exit 1
}

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

work_dir=$(mktemp -d "${TMPDIR:-/tmp}/subsite-monitor.XXXXXX") ||
    fail 'could not create a temporary directory'
staged_binary=''
cleanup() {
    rm -rf "$work_dir"
    if [ -n "$staged_binary" ]; then
        rm -f "$staged_binary"
    fi
}
trap cleanup EXIT
trap 'exit 1' HUP INT TERM

archive="$work_dir/$ASSET"
if command -v curl >/dev/null 2>&1; then
    curl --fail --location --silent --show-error \
        --output "$archive" "$ASSET_URL" || fail 'release download failed'
elif command -v wget >/dev/null 2>&1; then
    wget --quiet --output-document="$archive" "$ASSET_URL" ||
        fail 'release download failed'
else
    fail 'curl or wget is required to download the release'
fi

printf '%s  %s\n' "$ASSET_SHA256" "$archive" | sha256sum --check --status - ||
    fail 'release archive checksum verification failed'

mkdir -p "$work_dir/extracted"
tar -xzf "$archive" -C "$work_dir/extracted" \
    bin/subsite-monitor config/monitor.conf.example ||
    fail 'release archive could not be extracted'
binary="$work_dir/extracted/bin/subsite-monitor"
if [ ! -f "$binary" ] || [ ! -x "$binary" ]; then
    fail 'release archive does not contain the expected executable'
fi
config_template="$work_dir/extracted/config/monitor.conf.example"
if [ ! -f "$config_template" ]; then
    fail 'release archive does not contain the default configuration'
fi
if [ "$("$binary" --version)" != "subsite-monitor $VERSION" ]; then
    fail 'downloaded executable version does not match the release'
fi

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
else
    install -m 0600 "$config_template" "$CONFIG_FILE" ||
        fail "cannot install configuration file: $CONFIG_FILE"
fi

mkdir -p "$INSTALL_DIR" || fail "cannot create install directory: $INSTALL_DIR"
staged_binary=$(mktemp "$INSTALL_DIR/.subsite-monitor.XXXXXX") ||
    fail "cannot write to install directory: $INSTALL_DIR"
install -m 0755 "$binary" "$staged_binary" || fail 'could not stage executable'
mv -f "$staged_binary" "$INSTALL_DIR/subsite-monitor" ||
    fail 'could not install executable'
staged_binary=''

printf 'Installed subsite-monitor %s to %s/subsite-monitor\n' \
    "$VERSION" "$INSTALL_DIR"
