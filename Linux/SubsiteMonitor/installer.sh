#!/bin/sh
set -eu

VERSION='0.1.0-alpha.1'
ASSET='subsite-monitor-0.1.0-alpha.1-linux-x86_64-gnu.tar.gz'
ASSET_SHA256='17c42c282df629e00124eb439a6d0da76b88fec700a793af4afcadef48ad755d'
ASSET_URL="https://github.com/Dowscope/Utilities/releases/download/v${VERSION}/${ASSET}"
INSTALL_DIR=${SUBSITE_MONITOR_INSTALL_DIR:-/usr/local/bin}

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
tar -xzf "$archive" -C "$work_dir/extracted" bin/subsite-monitor ||
    fail 'release archive could not be extracted'
binary="$work_dir/extracted/bin/subsite-monitor"
if [ ! -f "$binary" ] || [ ! -x "$binary" ]; then
    fail 'release archive does not contain the expected executable'
fi
if [ "$("$binary" --version)" != "subsite-monitor $VERSION" ]; then
    fail 'downloaded executable version does not match the release'
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
