#!/usr/bin/env bash
set -euo pipefail

########################################
# Repo
########################################

REPO_BASE="https://raw.githubusercontent.com/Dowscope/Utilities/main/Linux/Debian"

RUN_TMP="/tmp/dowscope-setup"

USE_SUDO=true
MODE="install"
INSTALL_FREESWITCH=false

########################################
# Cleanup
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
        --freeswitch)
            INSTALL_FREESWITCH=true
            ;;
        *)
            echo "Unknown flag: $1"
            exit 1
            ;;
    esac
    shift
done

########################################
# Prepare runtime environment
########################################

echo "Preparing environment..."

rm -rf "$RUN_TMP"
mkdir -p "$RUN_TMP/lib"

########################################
# Download base files
########################################

BASE_FILES=(
    config.sh
    lib/core.sh
    lib/orchestrator.sh
)

echo "Downloading base files..."

for f in "${BASE_FILES[@]}"; do
    echo "Downloading $f"
    curl -fsSL "$REPO_BASE/$f" -o "$RUN_TMP/$f"
done

########################################
# Load configuration
########################################

source "$RUN_TMP/config.sh"

########################################
# Download modules
########################################

echo "Downloading modules..."

for module in "${MODULES[@]}"; do
    file="lib/${module}.sh"
    echo "Downloading $file"
    curl -fsSL "$REPO_BASE/$file" -o "$RUN_TMP/$file"
done

########################################
# Load core
########################################

source "$RUN_TMP/lib/core.sh"

########################################
# Load modules
########################################

echo "Loading modules..."

for module in "${MODULES[@]}"; do
    source "$RUN_TMP/lib/${module}.sh"
done

########################################
# Load orchestrator
########################################

source "$RUN_TMP/lib/orchestrator.sh"

########################################
# Export runtime flags
########################################

export USE_SUDO
export MODE
export INSTALL_FREESWITCH

########################################
# Execute main flow
########################################

if [[ "$MODE" == "install" ]]; then
    install_all
else
    remove_all
fi
