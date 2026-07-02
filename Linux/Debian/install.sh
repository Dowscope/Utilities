#!/usr/bin/env bash
set -euo pipefail

########################################
# Bootstrap Settings
########################################

REPO_BASE="https://raw.githubusercontent.com/Dowscope/Utilities/main/Linux/Debian"
BASE_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
TMP_DIR="/tmp/dowscope-bootstrap"
SETTINGS_FILE="$BASE_DIR/settings.sh"

MODE="install"
USE_SUDO=true
INSTALL_FREESWITCH=false
INSTALL_DEV=false

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

if [[ "$INSTALL_FREESWITCH" == true ]]; then
    validate_setting SIGNALWIRE_TOKEN
    validate_setting ESL_PASSWORD
    validate_setting SPEAKER_1000_PASSWORD
fi

if [[ "$INSTALL_DEV" == true ]]; then
    validate_setting GIT_USER_NAME
    validate_setting GIT_USER_EMAIL
    validate_setting GITHUB_SSH_KEY_NAME
fi

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

export MODE
export USE_SUDO
export INSTALL_FREESWITCH
export INSTALL_DEV

if [[ "$INSTALL_FREESWITCH" == true ]]; then
    export SIGNALWIRE_TOKEN
    export ESL_PASSWORD
    export SPEAKER_1000_PASSWORD
fi

if [[ "$INSTALL_DEV" == true ]]; then
    export GIT_USER_NAME
    export GIT_USER_EMAIL
    export GITHUB_SSH_KEY_NAME
    export GITHUB_SSH_KEY_SOURCE
fi

########################################
# Bootstrap
########################################

echo "Fetching latest setup.sh..."

curl -fsSL "$REPO_BASE/setup.sh" -o "$TMP_DIR/setup.sh"
chmod +x "$TMP_DIR/setup.sh"

echo "Launching setup.sh..."

exec "$TMP_DIR/setup.sh"
