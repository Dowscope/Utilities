#!/bin/bash
set -e

BACKUP_ROOT="/storage/main/backup"
BACKUPS=()
RESTORE_ITEMS=()
BACKUP_DIR=""
PARSED_SELECTION=()

print_menu() {
  local title="$1"
  shift
  local items=("$@")
  local term_width col_width cols i label

  term_width=$(tput cols 2>/dev/null || echo 100)
  col_width=32
  cols=$((term_width / col_width))
  ((cols < 1)) && cols=1

  echo
  echo "$title"
  echo

  for i in "${!items[@]}"; do
    label="${items[$i]}"
    printf "%-${col_width}s" "$((i + 1))) $label"

    if (( (i + 1) % cols == 0 )); then
      echo
    fi
  done

  echo
}

parse_selection() {
  local choice="$1"
  local max="$2"
  local selected=()
  local seen=()
  local part start end n

  choice="${choice// /}"

  IFS=',' read -ra parts <<< "$choice"

  for part in "${parts[@]}"; do
    if [[ "$part" =~ ^[0-9]+$ ]]; then
      selected+=("$part")
    elif [[ "$part" =~ ^([0-9]+)-([0-9]+)$ ]]; then
      start="${BASH_REMATCH[1]}"
      end="${BASH_REMATCH[2]}"

      if (( start > end )); then
        echo "Invalid range: $part"
        return 1
      fi

      for ((n=start; n<=end; n++)); do
        selected+=("$n")
      done
    else
      echo "Invalid choice: $part"
      return 1
    fi
  done

  PARSED_SELECTION=()

  for n in "${selected[@]}"; do
    if (( n < 1 || n > max )); then
      echo "Invalid number: $n"
      return 1
    fi

    if [ -z "${seen[$n]}" ]; then
      seen[$n]=1
      PARSED_SELECTION+=("$n")
    fi
  done
}

discover_backups() {
  if [ ! -d "$BACKUP_ROOT" ]; then
    echo "Backup root not found:"
    echo "$BACKUP_ROOT"
    exit 1
  fi

  mapfile -t BACKUPS < <(find "$BACKUP_ROOT" -mindepth 1 -maxdepth 1 -type d | sort -r)

  if [ ${#BACKUPS[@]} -eq 0 ]; then
    echo "No timestamped backups found in:"
    echo "$BACKUP_ROOT"
    exit 1
  fi
}

select_backup() {
  local labels=()
  local choice index

  for backup in "${BACKUPS[@]}"; do
    labels+=("$(basename "$backup")")
  done

  print_menu "Available backups:" "${labels[@]}"

  echo
  echo "Q) Quit"
  echo
  read -p "Choose backup [1-${#BACKUPS[@]}]: " choice

  case "$choice" in
    Q|q)
      exit 0
      ;;
  esac

  parse_selection "$choice" "${#BACKUPS[@]}"

  if [ ${#PARSED_SELECTION[@]} -ne 1 ]; then
    echo "Choose only one backup."
    exit 1
  fi

  index=$((PARSED_SELECTION[0] - 1))
  BACKUP_DIR="${BACKUPS[$index]}"
}

discover_restore_items() {
  RESTORE_ITEMS=()

  if [ ! -d "$BACKUP_DIR/home" ]; then
    echo "Selected backup does not contain:"
    echo "$BACKUP_DIR/home"
    exit 1
  fi

  while IFS= read -r item; do
    RESTORE_ITEMS+=("$(basename "$item")")
  done < <(find "$BACKUP_DIR/home" -mindepth 1 -maxdepth 1 | sort)

  if [ ${#RESTORE_ITEMS[@]} -eq 0 ]; then
    echo "No restore items found in:"
    echo "$BACKUP_DIR/home"
    exit 1
  fi
}

restore_item() {
  local item="$1"
  local src="$BACKUP_DIR/home/$item"

  if [ ! -e "$src" ]; then
    echo "Skipping missing: $item"
    return
  fi

  echo
  echo "Restoring: $item"
  rsync -aHAX --info=progress2 "$src" "$HOME/"
}

restore_all() {
  local item

  for item in "${RESTORE_ITEMS[@]}"; do
    restore_item "$item"
  done
}

restore_selected() {
  local n index item

  for n in "${PARSED_SELECTION[@]}"; do
    index=$((n - 1))
    item="${RESTORE_ITEMS[$index]}"
    restore_item "$item"
  done
}

restore_menu() {
  local choice

  print_menu "Restore items from $(basename "$BACKUP_DIR"):" "${RESTORE_ITEMS[@]}"

  echo
  echo "A) Restore all"
  echo "Q) Quit"
  echo
  read -p "Choose restore items [A, 1, 1-6, 1,4,6]: " choice

  case "$choice" in
    A|a)
      restore_all
      ;;
    Q|q)
      exit 0
      ;;
    *)
      parse_selection "$choice" "${#RESTORE_ITEMS[@]}"
      restore_selected
      ;;
  esac
}

fix_ownership() {
  echo
  echo "Fixing ownership..."
  sudo chown -R "$USER:$USER" "$HOME"
}

main() {
  discover_backups
  select_backup

  echo
  echo "Selected backup:"
  echo "$BACKUP_DIR"

  discover_restore_items
  restore_menu
  fix_ownership

  echo
  echo "Restore complete."
  echo "Log out and back in if you restored GNOME or application configs."
}

main
