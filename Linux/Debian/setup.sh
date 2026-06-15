#!/usr/bin/env bash
set -euo pipefail

REPO_BASE="https://raw.githubusercontent.com/Dowscope/Utilities/main/Linux/Debian"
TMP_DIR="/tmp/dowscope-setup"

USE_SUDO=true
MODE="install"

########################################
# Parse flags FIRST
########################################

while [[ $# -gt 0 ]]; do
    case "$1" in
        --remove)
            MODE="remove"
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
# Fetch modules
########################################

rm -rf "$TMP_DIR"
mkdir -p "$TMP_DIR/lib"

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
# Inject runtime state
########################################

export USE_SUDO
export MODE

########################################
# Run
########################################

if [[ "$MODE" == "install" ]]; then
    install_all
else
    remove_all
fi

rm -rf "$TMP_DIR"
