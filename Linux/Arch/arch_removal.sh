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
      echo
      echo "Required : $((required_kb / 1024)) MB"
      echo "Available: $((free_kb / 1024)) MB"
      echo

      echo "Largest directories:"
      du -xh --max-depth=1 "$HOME" 2>/dev/null | sort -hr | head -10

      echo
      echo "Largest files:"
      find "$HOME" -type f -printf '%s %p\n' 2>/dev/null \
        | sort -nr \
        | head -20 \
        | awk '{
            size=$1
            $1=""
            if(size>1073741824)
              printf "%.2f GB%s\n",size/1073741824,$0
            else if(size>1048576)
              printf "%.2f MB%s\n",size/1048576,$0
            else
              printf "%.2f KB%s\n",size/1024,$0
          }'

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
