#!/usr/bin/env bash
set -euo pipefail

########################################
# Bootstrap Settings
########################################

REPO_BASE="https://raw.githubusercontent.com/Dowscope/Utilities/main/Linux/Debian"
BASE_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
TMP_DIR="/tmp/dowscope-bootstrap"
SETTINGS_FILE="$BASE_DIR/settings.sh"

mkdir -p "$TMP_DIR"

########################################
# Helpers
########################################

validate_setting(){
    local name="$1"

    if [[ -z "${!name:-}" ]]; then
        echo "Missing setting: $name"
        MISSING_SETTINGS+=("$name")
    fi
}

download_file(){
    local file="$1"

    curl -fsSL "$REPO_BASE/$file" -o "$BASE_DIR/$file"
}

########################################
# Load Settings
########################################

if [[ ! -f "$SETTINGS_FILE" ]]; then

    echo "Downloading settings.sh..."

    download_file settings.sh

    echo
    echo "Settings file created:"
    echo "$SETTINGS_FILE"
    echo
    echo "Please edit this file and run the installer again."

    exit 1
fi

source "$SETTINGS_FILE"

MISSING_SETTINGS=()

validate_setting SIGNALWIRE_TOKEN

if [[ ${#MISSING_SETTINGS[@]} -gt 0 ]]; then

    echo
    echo "Please complete the following settings:"

    for setting in "${MISSING_SETTINGS[@]}"; do
        echo " - $setting"
    done

    echo
    echo "Edit:"
    echo "$SETTINGS_FILE"

    exit 1
fi

export SIGNALWIRE_TOKEN

########################################
# Bootstrap
########################################

echo "Fetching latest setup.sh..."

curl -fsSL "$REPO_BASE/setup.sh" -o "$TMP_DIR/setup.sh"
chmod +x "$TMP_DIR/setup.sh"

echo "Launching setup.sh..."

exec "$TMP_DIR/setup.sh" "$@"
