#!/usr/bin/env bash
set -euo pipefail

########################################
# Repo
########################################

REPO_BASE="https://raw.githubusercontent.com/Dowscope/Utilities/main/Linux/Debian"

BOOT_TMP="/tmp/dowscope-bootstrap"
RUN_TMP="/tmp/dowscope-setup"

USE_SUDO=true
MODE="install"

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
        --install)
            MODE="install"
            ;;
        --root)
            USE_SUDO=false
            ;;
        *)
            echo "Unknown flag: $1"
            exit 1
            ;;
    esac
    shift
done

########################################
# Helpers
########################################

run() {
    if [[ "$USE_SUDO" == true ]]; then
        sudo "$@"
    else
        "$@"
    fi
}

command_exists() {
    command -v "$1" >/dev/null 2>&1
}

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
