#!/bin/bash

set -e

MAIN_DRIVE="/dev/sdc1"
GAMES_DRIVE="/dev/sda1"

SELECTED_PROFILE=""
SELECTED_DESKTOP=""

ESSENTIAL_PACKAGES=(
  git
  less
  base-devel
  nodejs
  npm
  lazygit
  neovim
  tmux
  starship
  remmina
  freerdp
  firefox
  discord
  clang
  reflector
  bear
  glfw
  glm
  ripgrep
  rsync
  pacman-contrib
  steam
  python-requests
  python-beautifulsoup4
  tree-sitter
  otf-firamono-nerd
)

GNOME_PACKAGES=(
  wl-clipboard
)

CINNAMON_PACKAGES=(
  xclip
)

GNOME_REMOVE_PACKAGES=(
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

CINNAMON_REMOVE_PACKAGES=(
  hexchat
  hypnotix
  pix
  xed
)

GNOME_FAVORITES=(
  org.gnome.Nautilus.desktop
  firefox.desktop
  org.remmina.Remmina.desktop
  org.gnome.Console.desktop
)

CINNAMON_FAVORITES=(
  nemo.desktop
  firefox.desktop
  org.remmina.Remmina.desktop
  org.gnome.Console.desktop
)

enable_multilib() {
  echo "==> Enabling multilib repository..."

  if grep -Eq '^\[multilib\]' /etc/pacman.conf; then
    echo "multilib is already enabled."
    return
  fi

  sudo sed -i \
    '/^#\[multilib\]/,/^#Include = \/etc\/pacman.d\/mirrorlist/ s/^#//' \
    /etc/pacman.conf

  if grep -Eq '^\[multilib\]' /etc/pacman.conf; then
    echo "multilib enabled."
  else
    echo "Failed to enable multilib."
    exit 1
  fi
}

install_packages() {
  enable_multilib

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

  local tmpdir
  tmpdir=$(mktemp -d)

  git clone https://aur.archlinux.org/yay.git "$tmpdir/yay"

  pushd "$tmpdir/yay" >/dev/null
  makepkg -si --noconfirm
  popd >/dev/null

  rm -rf "$tmpdir"
}

setup_nvim() {
  echo "==> Setting up Neovim..."

  mkdir -p "$HOME/.config"

  if [ -d "$HOME/.config/nvim" ]; then
    rm -rf "$HOME/.config/nvim"
  fi

  git clone \
    https://github.com/Dowscope/NeoVim-Configs.git \
    "$HOME/.config/nvim"

  echo "==> Starting Neovim once to install configured plugins..."
  nvim --headless "+Lazy! sync" +qa || {
    echo "Warning: automatic Neovim plugin installation failed."
    echo "Open Neovim and run :Lazy sync manually."
  }

  echo "==> Updating Tree-sitter parsers..."
  nvim --headless "+TSUpdate" +qa || {
    echo "Warning: automatic Tree-sitter parser update failed."
    echo "Open Neovim and run :TSUpdate manually."
  }
}

setup_shell() {
  echo "==> Setting up shell..."

  local shell_rc="$HOME/.bashrc"

  mkdir -p "$HOME/.config"

  if ! grep -Fq 'eval "$(starship init bash)"' "$shell_rc"; then
    echo 'eval "$(starship init bash)"' >> "$shell_rc"
  fi

  starship preset catppuccin-powerline \
    -o "$HOME/.config/starship.toml"
}

setup_git() {
  echo "==> Setting up Git..."

  git config --global user.name "Timothy Dowling"
  git config --global user.email "timothy.dowling@me.com"
  git config --global core.editor "nvim"
  git config --global init.defaultBranch "main"
  git config --global fetch.prune true
}

setup_ssh() {
  echo "==> Setting up SSH key..."

  mkdir -p "$HOME/.ssh"
  chmod 700 "$HOME/.ssh"

  if [ ! -f "$HOME/.ssh/id_ed25519" ]; then
    ssh-keygen \
      -t ed25519 \
      -C "timothy_dowling@me.com" \
      -f "$HOME/.ssh/id_ed25519" \
      -N ""
  else
    echo "==> SSH key already exists."
  fi

  eval "$(ssh-agent -s)"
  ssh-add "$HOME/.ssh/id_ed25519"

  echo
  echo "==> Public SSH key:"
  cat "$HOME/.ssh/id_ed25519.pub"
}

install_aur_packages() {
  echo "==> Installing AUR packages..."
}

_build_favorites_value() {
  local favorites=()
  local output="["
  local first=true
  local app

  case "$SELECTED_DESKTOP" in
    gnome)
      favorites=("${GNOME_FAVORITES[@]}")
      ;;

    cinnamon)
      favorites=("${CINNAMON_FAVORITES[@]}")
      ;;

    *)
      echo "Unsupported desktop environment: $SELECTED_DESKTOP" >&2
      return 1
      ;;
  esac

  for app in "${favorites[@]}"; do
    if "$first"; then
      first=false
    else
      output+=", "
    fi

    output+="'$app'"
  done

  output+="]"

  printf '%s\n' "$output"
}

setup_desktop_favorites() {
  echo "==> Setting up $SELECTED_DESKTOP favorites..."

  local favorites
  favorites=$(_build_favorites_value)

  case "$SELECTED_DESKTOP" in
    gnome)
      if ! gsettings writable org.gnome.shell favorite-apps &>/dev/null; then
        echo "GNOME favorite-apps setting is unavailable."
        return
      fi

      gsettings set \
        org.gnome.shell \
        favorite-apps \
        "$favorites"
      ;;

    cinnamon)
      if ! gsettings writable org.cinnamon favorite-apps &>/dev/null; then
        echo "Cinnamon favorite-apps setting is unavailable."
        return
      fi

      gsettings set \
        org.cinnamon \
        favorite-apps \
        "$favorites"
      ;;

    *)
      echo "No favorites configuration exists for: $SELECTED_DESKTOP"
      ;;
  esac
}

remove_unwanted_packages() {
  echo "==> Checking for unwanted $SELECTED_DESKTOP packages..."

  local packages_to_check=()
  local installed_to_remove=()
  local pkg
  local answer

  case "$SELECTED_DESKTOP" in
    gnome)
      packages_to_check=("${GNOME_REMOVE_PACKAGES[@]}")
      ;;

    cinnamon)
      packages_to_check=("${CINNAMON_REMOVE_PACKAGES[@]}")
      ;;

    *)
      echo "No package-removal profile exists for: $SELECTED_DESKTOP"
      return
      ;;
  esac

  if [ "${#packages_to_check[@]}" -eq 0 ]; then
    echo "No unwanted packages configured for $SELECTED_DESKTOP."
    return
  fi

  for pkg in "${packages_to_check[@]}"; do
    if pacman -Q "$pkg" &>/dev/null; then
      installed_to_remove+=("$pkg")
    fi
  done

  if [ "${#installed_to_remove[@]}" -eq 0 ]; then
    echo "No unwanted $SELECTED_DESKTOP packages are installed."
    return
  fi

  echo
  echo "Packages to remove:"
  printf ' - %s\n' "${installed_to_remove[@]}"
  echo

  read -r -p "Remove these packages? [y/N] " answer

  if [[ "$answer" =~ ^[Yy]$ ]]; then
    sudo pacman -Rns --noconfirm "${installed_to_remove[@]}"
  else
    echo "Skipping package removal."
  fi
}

cleanup_packages() {
  echo "==> Checking for orphaned packages..."

  local orphans
  local answer

  orphans=$(pacman -Qdtq 2>/dev/null || true)

  if [ -n "$orphans" ]; then
    echo
    echo "Orphaned packages found:"
    printf '%s\n' "$orphans"
    echo

    read -r -p "Remove these orphaned packages? [y/N] " answer

    if [[ "$answer" =~ ^[Yy]$ ]]; then
      mapfile -t orphan_packages <<< "$orphans"
      sudo pacman -Rns --noconfirm "${orphan_packages[@]}"
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

  local main_uuid
  local games_uuid

  sudo mkdir -p /storage/main
  sudo mkdir -p /storage/games

  main_uuid=$(sudo blkid -s UUID -o value "$MAIN_DRIVE" || true)
  games_uuid=$(sudo blkid -s UUID -o value "$GAMES_DRIVE" || true)

  if [ -z "$main_uuid" ]; then
    echo "Failed to detect the main drive UUID: $MAIN_DRIVE"
    echo "Check drives with: lsblk -f"
    return 1
  fi

  if [ -z "$games_uuid" ]; then
    echo "Failed to detect the games drive UUID: $GAMES_DRIVE"
    echo "Check drives with: lsblk -f"
    return 1
  fi

  if ! grep -Fq "UUID=$main_uuid " /etc/fstab; then
    echo \
      "UUID=$main_uuid /storage/main ext4 defaults,noatime 0 2" |
      sudo tee -a /etc/fstab >/dev/null
  else
    echo "Main drive already exists in fstab."
  fi

  if ! grep -Fq "UUID=$games_uuid " /etc/fstab; then
    echo \
      "UUID=$games_uuid /storage/games ext4 defaults,noatime 0 2" |
      sudo tee -a /etc/fstab >/dev/null
  else
    echo "Games drive already exists in fstab."
  fi

  sudo systemctl daemon-reload
  sudo mount -a

  echo "Storage setup complete."
}

preflight_checks() {
  echo "==> Running preflight checks..."

  if [ "$EUID" -eq 0 ]; then
    echo "Do not run this installer as root."
    echo "Run it as your normal desktop user."
    exit 1
  fi

  if ! command -v sudo >/dev/null 2>&1; then
    echo "sudo is required but is not installed."
    exit 1
  fi

  if ! sudo -v; then
    echo "Unable to obtain sudo privileges."
    exit 1
  fi

  if ! command -v pacman >/dev/null 2>&1; then
    echo "This installer must be run on Arch Linux."
    exit 1
  fi
}

select_desktop_environment() {
  local desktop_options=(
    "GNOME"
    "Cinnamon"
    "Back"
  )

  echo
  echo "Select the installed desktop environment:"

  select desktop_option in "${desktop_options[@]}"; do
    case "$desktop_option" in
      "GNOME")
        SELECTED_DESKTOP="gnome"
        return 0
        ;;

      "Cinnamon")
        SELECTED_DESKTOP="cinnamon"
        return 0
        ;;

      "Back")
        return 1
        ;;

      *)
        echo "Invalid option. Try again."
        ;;
    esac
  done
}

install_desktop_packages() {
  echo "==> Installing packages for $SELECTED_DESKTOP..."

  case "$SELECTED_DESKTOP" in
    gnome)
      sudo pacman -S --needed --noconfirm "${GNOME_PACKAGES[@]}"
      ;;

    cinnamon)
      sudo pacman -S --needed --noconfirm "${CINNAMON_PACKAGES[@]}"
      ;;

    *)
      echo "No desktop-specific package profile exists for: $SELECTED_DESKTOP"
      return 1
      ;;
  esac
}

main_desktop() {
  echo
  echo "Running setup for MAIN DESKTOP"
  echo "Desktop environment: $SELECTED_DESKTOP"
  echo

  install_packages
  install_desktop_packages
  install_yay
  setup_nvim
  install_aur_packages
  setup_shell
  setup_git
  setup_ssh
  setup_desktop_favorites
  remove_unwanted_packages
  cleanup_packages
  setup_storage_drives

  echo
  echo "Main desktop setup complete."
}

gaming_desktop() {
  echo
  echo "Running setup for GAMING DESKTOP"
  echo "Desktop environment: $SELECTED_DESKTOP"
  echo

  echo "Gaming Desktop setup has not been configured yet."
}

show_main_menu() {
  local profile_options=(
    "Main Desktop"
    "Gaming Desktop"
    "Exit"
  )

  while true; do
    echo
    echo "Select the computer profile:"

    select profile_option in "${profile_options[@]}"; do
      case "$profile_option" in
        "Main Desktop")
          SELECTED_PROFILE="main"

          if select_desktop_environment; then
            main_desktop
            return
          fi

          break
          ;;

        "Gaming Desktop")
          SELECTED_PROFILE="gaming"

          if select_desktop_environment; then
            gaming_desktop
            return
          fi

          break
          ;;

        "Exit")
          echo "Exiting..."
          return
          ;;

        *)
          echo "Invalid option. Try again."
          ;;
      esac
    done
  done
}

preflight_checks
show_main_menu
