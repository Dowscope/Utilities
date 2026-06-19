#!/usr/bin/env bash
set -euo pipefail

echo "SETUP ESL_PASSWORD=[${ESL_PASSWORD:-}]"

########################################
# Repo
########################################

REPO_BASE="https://raw.githubusercontent.com/Dowscope/Utilities/main/Linux/Debian"
RUN_TMP="/tmp/dowscope-setup"

########################################
# Cleanup
########################################

cleanup(){
    rm -rf "$RUN_TMP"
}

trap cleanup EXIT INT TERM

########################################
# Prepare runtime environment
########################################

echo "Preparing environment..."

rm -rf "$RUN_TMP"
mkdir -p "$RUN_TMP/lib"

########################################
# Download config
########################################

echo "Downloading config.sh"

curl -fsSL "$REPO_BASE/config.sh" -o "$RUN_TMP/config.sh"

source "$RUN_TMP/config.sh"

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
        --sudo)
            USE_SUDO=true
            ;;
        --*)
            flag="${1#--}"
            var="INSTALL_${flag^^}"
            if [[ -v "$var" ]]; then
                printf -v "$var" true
            else
                echo "Unknown flag: $1"
                exit 1
            fi
            ;;
        *)
            echo "Unknown argument: $1"
            exit 1
            ;;
    esac
    shift
done

########################################
# Build module lists
########################################

ALL_MODULES=("${MODULES[@]}" "${OPTIONAL_MODULES[@]}")

for module in "${OPTIONAL_MODULES[@]}"; do
    flag="INSTALL_${module^^}"
    if [[ "${!flag:-false}" == true ]]; then
        MODULES+=("$module")
    fi
done

########################################
# Export runtime variables
########################################

export USE_SUDO
export MODE
export RUN_TMP
export REPO_BASE

########################################
# Download base files
########################################

BASE_FILES=(
    lib/core.sh
    lib/orchestrator.sh
)

echo "Downloading base files..."

for f in "${BASE_FILES[@]}"; do
    echo "Downloading $f"
    curl -fsSL "$REPO_BASE/$f" -o "$RUN_TMP/$f"
done

########################################
# Download modules
########################################

echo "Downloading modules..."

for module in "${ALL_MODULES[@]}"; do
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

for module in "${ALL_MODULES[@]}"; do
    source "$RUN_TMP/lib/${module}.sh"
done

########################################
# Load orchestrator
########################################

source "$RUN_TMP/lib/orchestrator.sh"

########################################
# Execute
########################################

if [[ "$MODE" == "install" ]]; then
    install_all
else
    remove_all
fi
