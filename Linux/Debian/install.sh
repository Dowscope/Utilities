#!/usr/bin/env bash
set -euo pipefail

########################################
# User Settings
########################################

SIGNALWIRE_TOKEN=""

########################################
# Validate Settings
########################################

if [[ -z "$SIGNALWIRE_TOKEN" ]]; then
    echo "Missing setting: SIGNALWIRE_TOKEN"
    echo "Please edit install.sh and add your SignalWire token."
    exit 1
fi

########################################
# Bootstrap
########################################

REPO_BASE="https://raw.githubusercontent.com/Dowscope/Utilities/main/Linux/Debian"
TMP_DIR="/tmp/dowscope-bootstrap"

mkdir -p "$TMP_DIR"

export SIGNALWIRE_TOKEN

echo "Fetching latest setup.sh..."

curl -fsSL "$REPO_BASE/setup.sh" -o "$TMP_DIR/setup.sh"
chmod +x "$TMP_DIR/setup.sh"

echo "Launching setup.sh..."

exec "$TMP_DIR/setup.sh" "$@"
