#!/usr/bin/env bash
set -euo pipefail

########################################
# Version + Repo
########################################

SCRIPT_VERSION="1.0.2"
REPO_BASE="https://raw.githubusercontent.com/Dowscope/Utilities/main/Linux/Debian"

BOOT_TMP="/tmp/dowscope-bootstrap"
RUN_TMP="/tmp/dowscope-setup"

USE_SUDO=true
MODE="install"
SKIP_UPDATE=false

# Preserve original args for re-exec
original_args=("$@")

########################################
# Cleanup (runtime only)
########################################

cleanup() {
    rm -rf "$RUN_TMP"
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

    mkdir -p "$BOOT_TMP"
    local tmp_file="$BOOT_TMP/setup.sh"

    curl -fsSL "$REPO_BASE/setup.sh" -o "$tmp_file"
    chmod +x "$tmp_file"

    echo "Restarting updated script..."

    exec "$tmp_file" "${original_args[@]}"
}

########################################
# RUN UPDATE FIRST (critical)
########################################

check_update "$@"

########################################
# Prepare runtime environment
########################################

echo "Preparing environment..."

rm -rf "$RUN_TMP"
mkdir -p "$RUN_TMP/lib"

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
    mkdir -p "$RUN_TMP/$(dirname "$f")"
    curl -fsSL "$REPO_BASE/$f" -o "$RUN_TMP/$f"
done

########################################
# Load modules
########################################

source "$RUN_TMP/config.sh"
source "$RUN_TMP/lib/core.sh"
source "$RUN_TMP/lib/packages.sh"
source "$RUN_TMP/lib/node.sh"
source "$RUN_TMP/lib/neovim.sh"
source "$RUN_TMP/lib/treesitter.sh"
source "$RUN_TMP/lib/orchestrator.sh"

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
