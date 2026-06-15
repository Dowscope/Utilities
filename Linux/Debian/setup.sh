#!/usr/bin/env bash

set -euo pipefail

########################################
# Configuration
########################################

LOG_FILE="setup.log"
USE_SUDO=true
MODE="install"

########################################
# Packages
########################################

CORE_PACKAGES=(
    git
    curl
)

USER_PACKAGES=(
)

########################################
# Logging
########################################

exec > >(tee -i "$LOG_FILE")
exec 2>&1

########################################
# Helpers
########################################

run() {
    if [[ "$USE_SUDO" == true ]]; then
        sudo "$@"
    else
        "$@"
    fi
}

command_exists() {
    command -v "$1" >/dev/null 2>&1
}

log_section() {
    echo
    echo "========================================"
    echo " $1"
    echo "========================================"
}

usage() {
    cat <<EOF
Usage: $0 [options]

Options:
  --rootuser    Run commands without sudo
  --remove      Remove user-installed packages and configs
  --help        Show this help

Examples:
  $0
  $0 --remove
  $0 --rootuser
EOF
}

########################################
# Package Functions
########################################

install_packages() {
    local packages=("$@")

    for pkg in "${packages[@]}"; do
        if dpkg -s "$pkg" >/dev/null 2>&1; then
            echo "$pkg already installed"
        else
            echo "Installing $pkg..."
            run apt install -y "$pkg"
        fi
    done
}

remove_packages() {
    local packages=("$@")

    for pkg in "${packages[@]}"; do
        if dpkg -s "$pkg" >/dev/null 2>&1; then
            echo "Removing $pkg..."
            run apt remove -y "$pkg"
        else
            echo "$pkg not installed"
        fi
    done

    run apt autoremove -y
}

########################################
# Neovim
########################################

install_neovim() {

    log_section "Installing Neovim"

    if command_exists nvim; then
        echo "Neovim already installed"
        return
    fi

    local archive="nvim-linux-x86_64.tar.gz"
    local directory="nvim-linux-x86_64"

    curl -LO \
        https://github.com/neovim/neovim/releases/latest/download/${archive}

    tar xzf "$archive"

    run cp -a "${directory}/." /usr/local/

    rm -rf "$archive" "$directory"

    echo
    echo "Installed version:"
    nvim --version | head -n 1 || true
}

remove_neovim() {

    log_section "Removing Neovim"

    run rm -f /usr/local/bin/nvim
    run rm -rf /usr/local/lib/nvim
    run rm -rf /usr/local/share/nvim

    echo "Neovim removed"
}

########################################
# Neovim Config
########################################

install_nvim_config() {

    log_section "Installing Neovim Config"

    local nvim_dir="$HOME/.config/nvim"

    if [[ -d "$nvim_dir" ]]; then
        local backup="${nvim_dir}.bak.$(date +%s)"
        echo "Backing up existing config to:"
        echo "  $backup"
        mv "$nvim_dir" "$backup"
    fi

    mkdir -p "$HOME/.config"

    git clone \
        https://github.com/dowscope/Neovim-Configs.git \
        "$nvim_dir"
}

remove_nvim_config() {

    log_section "Removing Neovim Config"

    local nvim_dir="$HOME/.config/nvim"

    if [[ -d "$nvim_dir" ]]; then
        rm -rf "$nvim_dir"
        echo "Removed $nvim_dir"
    else
        echo "No Neovim config found"
    fi
}

########################################
# Install Mode
########################################

install_mode() {

    log_section "Starting INSTALL"

    echo "Updating package lists..."
    run apt update

    echo "Upgrading packages..."
    run apt upgrade -y

    log_section "Installing Core Packages"
    install_packages "${CORE_PACKAGES[@]}"

    log_section "Installing User Packages"
    install_packages "${USER_PACKAGES[@]}"

    install_neovim
    install_nvim_config

    log_section "INSTALL COMPLETE"
}

########################################
# Remove Mode
########################################

remove_mode() {

    log_section "Starting REMOVE"

    remove_neovim
    remove_nvim_config

    log_section "Removing User Packages"
    remove_packages "${USER_PACKAGES[@]}"

    log_section "REMOVE COMPLETE"
}

########################################
# Parse Arguments
########################################

while [[ $# -gt 0 ]]; do
    case "$1" in
        --rootuser)
            USE_SUDO=false
            ;;
        --remove)
            MODE="remove"
            ;;
        --help|-h)
            usage
            exit 0
            ;;
        *)
            echo "Unknown option: $1"
            usage
            exit 1
            ;;
    esac
    shift
done

########################################
# Main
########################################

case "$MODE" in
    install)
        install_mode
        ;;
    remove)
        remove_mode
        ;;
    *)
        echo "Unknown mode: $MODE"
        exit 1
        ;;
esac
