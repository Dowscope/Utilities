#!/bin/bash
set -e

BACKUP_ROOT="/storage/main/backup"
TIMESTAMP="$(date +%Y-%m-%d_%H-%M-%S)"
BACKUP_DIR="$BACKUP_ROOT/$TIMESTAMP"
SOURCE_HOME="$HOME"
RESTORE_SCRIPT_URL="https://raw.githubusercontent.com/Dowscope/Utilities/refs/heads/main/Linux/Arch/arch_restore.sh"

EXCLUDES=(
  ".cache"
  ".local/share/Trash"
  ".local/share/Steam/steamapps"
  ".local/share/Steam/shadercache"
  ".local/share/Steam/logs"
  ".local/share/Steam/dumps"
  ".local/share/Steam/crashreports"
)

build_exclude_args() {
  EXCLUDE_ARGS=()

  for exclude in "${EXCLUDES[@]}"; do
    EXCLUDE_ARGS+=(--exclude="$exclude")
  done
}

get_backup_size_kb() {
  du -sk "${EXCLUDE_ARGS[@]}" "$SOURCE_HOME" | awk '{print $1}'
}

get_free_kb() {
  df -Pk "$1" | awk 'NR==2 {print $4}'
}

show_largest_items() {
  echo
  echo "Largest directories in home:"
  du -xh --max-depth=1 "$SOURCE_HOME" 2>/dev/null | sort -hr | head -15

  echo
  echo "Largest files in home:"
  find "$SOURCE_HOME" -type f -printf '%s %p\n' 2>/dev/null \
    | sort -nr \
    | head -20 \
    | awk '{
        size=$1
        $1=""
        if(size>=1073741824)
          printf "%.2f GB%s\n",size/1073741824,$0
        else if(size>=1048576)
          printf "%.2f MB%s\n",size/1048576,$0
        else
          printf "%.2f KB%s\n",size/1024,$0
      }'
}

main() {
  echo "==> Preparing home backup..."

  mkdir -p "$BACKUP_ROOT"
  mkdir -p "$BACKUP_DIR"

  build_exclude_args

  required_kb=$(get_backup_size_kb)
  free_kb=$(get_free_kb "$BACKUP_ROOT")

  echo "Backup size required: $((required_kb / 1024)) MB"
  echo "Backup free space:    $((free_kb / 1024)) MB"

  if [ "$free_kb" -lt "$required_kb" ]; then
    echo
    echo "ERROR: Not enough free space."
    echo "Required : $((required_kb / 1024)) MB"
    echo "Available: $((free_kb / 1024)) MB"
    show_largest_items
    exit 1
  fi

  echo
  echo "==> Backing up home directory..."
  echo "From: $SOURCE_HOME/"
  echo "To:   $BACKUP_DIR/home/"
  echo

  rsync -aHAX --info=progress2 \
    "${EXCLUDE_ARGS[@]}" \
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
