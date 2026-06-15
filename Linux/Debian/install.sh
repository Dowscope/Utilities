#!/usr/bin/env bash
set -euo pipefail

REPO_BASE="https://raw.githubusercontent.com/Dowscope/Utilities/main/Linux/Debian"
TMP_DIR="/tmp/dowscope-bootstrap"

mkdir -p "$TMP_DIR"

echo "Fetching latest setup.sh..."

curl -fsSL "$REPO_BASE/setup.sh" -o "$TMP_DIR/setup.sh"
chmod +x "$TMP_DIR/setup.sh"

echo "Launching setup.sh..."

exec "$TMP_DIR/setup.sh" "$@"
