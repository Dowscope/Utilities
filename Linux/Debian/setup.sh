#!/usr/bin/env bash
set -euo pipefail

########################################
# Version + Repo
########################################

SCRIPT_VERSION="1.0.1"
REPO_BASE="https://raw.githubusercontent.com/Dowscope/Utilities/main/Linux/Debian"

TMP_DIR="/tmp/dowscope-setup"

USE_SUDO=true
MODE="install"
SKIP_UPDATE=false

########################################
# Cleanup (always runs on exit/crash)
########################################

cleanup() {
    rm -rf "$TMP_DIR"
}

trap cleanup EXIT INT TERM

########################################
# Parse flags
########################################

while [[ $# -gt 0 ]]; do
    case "$1" in
        --remove)
            MODE="remove"
            ;;
        --root)
            USE_SUDO=false
            ;;
        --no-update)
            SKIP_UPDATE=true
            ;;
        *)
            echo "Unknown flag: $1"
            exit 1
            ;;
    esac
    shift
done

########################################
# Version check + auto update
########################################

check_update() {

    if [[ "$SKIP_UPDATE" == true ]]; then
        return
    fi

    local remote_version
    remote_version=$(curl -fsSL "$REPO_BASE/VERSION" || true)

    if [[ -z "$remote_version" ]]; then
        echo "Unable to check version (offline or missing VERSION file)"
        return
    fi

    if [[ "$remote_version" == "$SCRIPT_VERSION" ]]; then
        return
    fi

    echo
    echo "======================================"
    echo " Update available"
    echo " Local:  $SCRIPT_VERSION"
    echo " Remote: $remote_version"
    echo "======================================"
    echo

    read -rp "Update setup.sh now? [Y/n]: " choice
    choice="${choice:-Y}"

    if [[ "$choice" =~ ^[Nn]$ ]]; then
        echo "Skipping update..."
        return
    fi

    echo "Updating setup.sh..."

    mkdir -p "$TMP_DIR"
    local tmp_file="$TMP_DIR/setup.sh"

    curl -fsSL "$REPO_BASE/setup.sh" -o "$tmp_file"
    chmod +x "$tmp_file"

    echo "Restarting updated script..."

    exec "$tmp_file" "$@"
}

########################################
# Run update check FIRST
########################################

check_update "$@"

########################################
# Prepare temp directory (fresh every run)
########################################

echo "Preparing environment..."

rm -rf "$TMP_DIR"
mkdir -p "$TMP_DIR/lib"

########################################
# Download modules
########################################

files=(
    config.sh
    lib/core.sh
    lib/packages.sh
    lib/node.sh
    lib/neovim.sh
    lib/treesitter.sh
    lib/orchestrator.sh
)

echo "Downloading modules..."

for f in "${files[@]}"; do
    mkdir -p "$TMP_DIR/$(dirname "$f")"
    curl -fsSL "$REPO_BASE/$f" -o "$TMP_DIR/$f"
done

########################################
# Load modules
########################################

source "$TMP_DIR/config.sh"
source "$TMP_DIR/lib/core.sh"
source "$TMP_DIR/lib/packages.sh"
source "$TMP_DIR/lib/node.sh"
source "$TMP_DIR/lib/neovim.sh"
source "$TMP_DIR/lib/treesitter.sh"
source "$TMP_DIR/lib/orchestrator.sh"

########################################
# Export runtime flags
########################################

export USE_SUDO
export MODE

########################################
# Execute main flow
########################################

if [[ "$MODE" == "install" ]]; then
    install_all
else
    remove_all
fi
