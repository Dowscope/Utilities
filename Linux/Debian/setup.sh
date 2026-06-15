#!/usr/bin/env bash

set -e

#########################################
# Logging
#########################################

LOG_FILE="setup.log"
exec > >(tee -i "$LOG_FILE")
exec 2>&1

#########################################
# Flags
#########################################

USE_SUDO=true
MODE="install"   # default

for arg in "$@"; do
    case $arg in
        --rootuser)
            USE_SUDO=false
            shift
            ;;
        --remove)
            MODE="remove"
            shift
            ;;
    esac
done

#########################################
# Helpers
#########################################

run() {
    if [ "$USE_SUDO" = true ]; then
        sudo "$@"
    else
        "$@"
    fi
}

command_exists() {
    command -v "$1" >/dev/null 2>&1
}

#########################################
# INSTALL FUNCTION
#########################################

install() {

echo "================================="
echo " Starting INSTALL"
echo "================================="

echo "==== Updating system ===="
run apt update
run apt upgrade -y

echo "==== Installing core packages ===="

PACKAGES=(
    git
    curl
    unzip
)

for pkg in "${PACKAGES[@]}"; do
    if ! dpkg -s "$pkg" >/dev/null 2>&1; then
        echo "Installing $pkg..."
        run apt install -y "$pkg"
    else
        echo "$pkg already installed"
    fi
done

#########################################
# Install Neovim 
#########################################

echo "==== Installing Neovim (tarball) ===="

if ! command_exists nvim; then
    curl -LO https://github.com/neovim/neovim/releases/latest/download/nvim-linux-x86_64.tar.gz
    tar xzf nvim-linux-x86_64.tar.gz

    run cp -r nvim-linux-x86_64/* /usr/local/

    rm -rf nvim-linux-x86_64*
else
    echo "Neovim already installed"
fi

echo "Neovim version:"
nvim --version | head -n 1 || true

#########################################
# Neovim Config
#########################################

echo "==== Setting up Neovim config ===="

NVIM_DIR="$HOME/.config/nvim"

if [ -d "$NVIM_DIR" ]; then
    echo "Backing up existing config..."
    mv "$NVIM_DIR" "$NVIM_DIR.bak.$(date +%s)"
fi

git clone https://github.com/dowscope/Neovim-Configs.git "$NVIM_DIR"

#########################################

echo "INSTALL COMPLETE"
}

#########################################
# REMOVE FUNCTION
#########################################

remove() {

echo "================================="
echo " Starting REMOVE"
echo "================================="

echo "==== Removing Neovim ===="

# Remove binary
if command_exists nvim; then
    run rm -f /usr/local/bin/nvim
    echo "Removed /usr/local/bin/nvim"
else
    echo "Neovim not found"
fi

# Remove shared files (tarball install)
run rm -rf /usr/local/lib/nvim || true
run rm -rf /usr/local/share/nvim || true

#########################################
# Remove config
#########################################

echo "==== Removing Neovim config ===="

NVIM_DIR="$HOME/.config/nvim"

if [ -d "$NVIM_DIR" ]; then
    rm -rf "$NVIM_DIR"
    echo "Removed config"
else
    echo "No config found"
fi

#########################################

echo "REMOVE COMPLETE"
}

#########################################
# EXECUTE MODE
#########################################

if [ "$MODE" = "install" ]; then
    install
elif [ "$MODE" = "remove" ]; then
    remove
else
    echo "Unknown mode"
    exit 1
fi
