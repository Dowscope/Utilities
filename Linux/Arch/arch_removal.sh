#!/bin/bash

set -e

BACKUP_ROOT="/storage/main/backup"
TIMESTAMP="$(date +%Y-%m-%d_%H-%M-%d_%H-%M-%S)"
BACKUP_DIR="$BACKUP_ROOT/$TIMESTAMP"
SOURCE_HOME="$HOME"

EXCLUDES=(
  ".cache/"
  ".local/share/Trash/"
  ".steam/steam/steamapps/"
)

RESTORE_SCRIPT_URL="https://raw.githubusercontent.com/Dowscope/Utilities/refs/heads/main/Linux/Arch/arch_restore.sh"

get_size_kb() {
  du -sk "$1" | awk '{print $1}'
}

get_free_kb() {
  df -Pk "$1" | awk 'NR==2 {print $4}'
}

main() {
  echo "==> Preparing home backup..."

  mkdir -p "$BACKUP_ROOT"

  required_kb=$(get_size_kb "$SOURCE_HOME")
  free_kb=$(get_free_kb "$BACKUP_ROOT")

  echo "Home size required: $((required_kb / 1024)) MB"
  echo "Backup free space:  $((free_kb / 1024)) MB"

  if [ "$free_kb" -lt "$required_kb" ]; then
    echo
    echo "ERROR: Not enough free space."
    echo "Required : $((required_kb / 1024)) MB"
    echo "Available: $((free_kb / 1024)) MB"
    exit 1
  fi

  mkdir -p "$BACKUP_DIR"

  echo
  echo "==> Backing up home directory..."
  echo "From: $SOURCE_HOME/"
  echo "To:   $BACKUP_DIR/home/"
  echo

  rsync -aHAX --info=progress2 \
    "${EXCLUDES[@]/#/--exclude=}" \
    "$SOURCE_HOME/" "$BACKUP_DIR/home/"

  echo
  echo "========================================"
  echo " BACKUP COMPLETE"
  echo "========================================"
  echo
  echo "Backup saved to:"
  echo "  $BACKUP_DIR"
  echo
  echo "To restore after reinstalling Arch:"
  echo
  echo "curl -fsSL \"$RESTORE_SCRIPT_URL\" -o arch_restore.sh"
  echo "chmod +x arch_restore.sh"
  echo "./arch_restore.sh"
  echo
}

main
