#!/bin/bash

set -e

MAIN_DRIVE="/dev/sdc1"
GAMES_DRIVE="/dev/sda1"

REMOVE_PACKAGES=(
  epiphany
  gnome-calendar
  gnome-contacts
  gnome-maps
  gnome-user-docs
  totem
  yelp
  simple-scan
  cheese
  seahorse
  gnome-weather
  gnome-clocks
  gnome-connections
)

ESSENTIAL_PACKAGES=(
  git
  base-devel
  nodejs
  npm
  lazygit
  neovim
  tmux
  starship
  remmina
  firefox
  discord
  clang
  reflector
  bear
  glfw
  glm
  wl-clipboard
  ripgrep
  rsync
  pacman-contrib
)

install_packages() {
  echo "==> Updating system..."
  sudo pacman -Syu --noconfirm

  echo "==> Installing essential packages..."
  sudo pacman -S --needed --noconfirm "${ESSENTIAL_PACKAGES[@]}"
}

install_yay() {
  if command -v yay &>/dev/null; then
    echo "==> yay is already installed."
    return
  fi

  echo "==> Installing yay..."
  tmpdir=$(mktemp -d)

  git clone https://aur.archlinux.org/yay.git "$tmpdir/yay"
  cd "$tmpdir/yay"
  makepkg -si --noconfirm
  cd - >/dev/null

  rm -rf "$tmpdir"
}

setup_nvim() {
  echo "==> Setting up Neovim..."

  mkdir -p "$HOME/.config"

  if [ -d "$HOME/.config/nvim" ]; then
    rm -rf "$HOME/.config/nvim"
  fi

  git clone https://github.com/Dowscope/NeoVim-Configs.git "$HOME/.config/nvim"
}

setup_shell() {
  echo "==> Setting up shell..."

  SHELL_RC="$HOME/.bashrc"

  if ! grep -q 'eval "$(starship init bash)"' "$SHELL_RC"; then
    echo 'eval "$(starship init bash)"' >> "$SHELL_RC"
  fi

  starship preset catppuccin-powerline -o "$HOME/.config/starship.toml"
}

setup_git() {
  echo "==> Setting up Git..."

  git config --global user.name "Timothy Dowling"
  git config --global user.email "timothy.dowling@me.com"
}

setup_ssh() {
  echo "==> Setting up SSH key..."

  mkdir -p "$HOME/.ssh"

  if [ ! -f "$HOME/.ssh/id_ed25519" ]; then
    ssh-keygen -t ed25519 -C "timothy_dowling@me.com" -f "$HOME/.ssh/id_ed25519" -N ""
  else
    echo "==> SSH key already exists."
  fi

  eval "$(ssh-agent -s)"
  ssh-add "$HOME/.ssh/id_ed25519"

  echo "==> Public SSH key:"
  cat "$HOME/.ssh/id_ed25519.pub"
}

install_aur_packages() {
  echo "==> Installing AUR packages..."

  yay -S --needed --noconfirm otf-firamono-nerd
}

remove_unwanted_packages() {
  echo "==> Removing unwanted packages..."

  installed_to_remove=()

  for pkg in "${REMOVE_PACKAGES[@]}"; do
    if pacman -Q "$pkg" &>/dev/null; then
      installed_to_remove+=("$pkg")
    fi
  done

  if [ ${#installed_to_remove[@]} -eq 0 ]; then
    echo "No unwanted packages installed."
    return
  fi

  echo "Packages to remove:"
  printf ' - %s\n' "${installed_to_remove[@]}"

  read -p "Remove these packages? [y/N] " answer
  if [[ "$answer" =~ ^[Yy]$ ]]; then
    sudo pacman -Rns --noconfirm "${installed_to_remove[@]}"
  else
    echo "Skipping package removal."
  fi
}

cleanup_packages() {
  echo "==> Checking for orphaned packages..."

  orphans=$(pacman -Qdtq 2>/dev/null || true)

  if [ -n "$orphans" ]; then
    echo "Orphaned packages found:"
    echo "$orphans"

    read -p "Remove these orphaned packages? [y/N] " answer
    if [[ "$answer" =~ ^[Yy]$ ]]; then
      sudo pacman -Rns --noconfirm $orphans
    else
      echo "Skipping orphan removal."
    fi
  else
    echo "No orphaned packages found."
  fi

  echo "==> Cleaning package cache..."
  sudo rm -f /var/cache/pacman/pkg/download-* 2>/dev/null || true
  sudo paccache -rk2 || true
  sudo paccache -ruk0 || true
}

setup_storage_drives() {
  echo "==> Setting up storage drives..."

  sudo mkdir -p /storage/main
  sudo mkdir -p /storage/games

  MAIN_UUID=$(blkid -s UUID -o value "$MAIN_DRIVE" || true)
  GAMES_UUID=$(blkid -s UUID -o value "$GAMES_DRIVE" || true)

  if [ -z "$MAIN_UUID" ] || [ -z "$GAMES_UUID" ]; then
    echo "Failed to detect drive UUIDs."
    echo "Check drives with: lsblk -f"
    return 1
  fi

  if ! grep -q "$MAIN_UUID" /etc/fstab; then
    echo "UUID=$MAIN_UUID /storage/main ext4 defaults,noatime 0 2" | sudo tee -a /etc/fstab
  else
    echo "Main drive already exists in fstab."
  fi

  if ! grep -q "$GAMES_UUID" /etc/fstab; then
    echo "UUID=$GAMES_UUID /storage/games ext4 defaults,noatime 0 2" | sudo tee -a /etc/fstab
  else
    echo "Games drive already exists in fstab."
  fi

  sudo systemctl daemon-reload
  sudo mount -a

  echo "Storage setup complete."
}

main_desktop() {
  echo "Running setup for MAIN DESKTOP"

  install_packages
  install_yay
  setup_nvim
  install_aur_packages
  setup_shell
  setup_git
  setup_ssh
  remove_unwanted_packages
  cleanup_packages
  setup_storage_drives
}

gaming_desktop() {
  echo "Running setup for GAMING DESKTOP"
}

echo "Select the option:"
options=("Main Desktop" "Gaming Desktop" "Exit")

select opt in "${options[@]}"; do
  case $opt in
    "Main Desktop")
      echo "You have chosen MAIN DESKTOP"
      main_desktop
      break
      ;;
    "Gaming Desktop")
      echo "You have chosen GAMING DESKTOP"
      gaming_desktop
      break
      ;;
    "Exit")
      echo "Exiting..."
      break
      ;;
    *)
      echo "Invalid option. Try again."
      ;;
  esac
done
