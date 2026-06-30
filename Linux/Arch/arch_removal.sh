#!/bin/bash

set -e

BACKUP_ROOT="/storage/main/backup"
TIMESTAMP="$(date +%Y-%m-%d_%H-%M-%S)"
BACKUP_DIR="$BACKUP_ROOT/$TIMESTAMP"
SOURCE_HOME="$HOME"

EXCLUDES=(
  ".cache/"
  ".local/share/Trash/"
  ".steam/steam/steamapps/"
)

get_size_kb() {
  du -sk "$1" | awk '{print $1}'
}

get_free_kb() {
  df -Pk "$1" | awk 'NR==2 {print $4}'
}

main() {
  echo "==> Preparing full home backup..."

  mkdir -p "$BACKUP_ROOT"

  required_kb=$(get_size_kb "$SOURCE_HOME")
  free_kb=$(get_free_kb "$BACKUP_ROOT")

  echo "Home size required: $((required_kb / 1024)) MB"
  echo "Backup free space:  $((free_kb / 1024)) MB"

  if [ "$free_kb" -lt "$required_kb" ]; then
    echo "Not enough free space."
    echo "Required: $((required_kb / 1024)) MB"
    echo "Available: $((free_kb / 1024)) MB"
    exit 1
  fi

  mkdir -p "$BACKUP_DIR"

  echo "==> Backing up entire home folder..."
  echo "From: $SOURCE_HOME/"
  echo "To:   $BACKUP_DIR/home/"

  rsync -aHAX --numeric-ids --info=progress2 \
    "${EXCLUDES[@]/#/--exclude=}" \
    "$SOURCE_HOME/" "$BACKUP_DIR/home/"

  cat > "$BACKUP_DIR/restore.sh" <<'EOF'
#!/bin/bash

set -e

BACKUP_DIR="$(cd "$(dirname "$0")" && pwd)"

echo "Restoring home backup from:"
echo "$BACKUP_DIR/home/"

rsync -aHAX --info=progress2 "$BACKUP_DIR/home/" "$HOME/"

echo "Restore complete."
EOF

  chmod +x "$BACKUP_DIR/restore.sh"

  echo "==> Backup complete:"
  echo "$BACKUP_DIR"
  echo
  echo "Restore later with:"
  echo "$BACKUP_DIR/restore.sh"
}

main
