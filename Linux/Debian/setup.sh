#!/usr/bin/bash

set -e

LOG_FILE="setup.log"
exec > >(tee -i "$LOG_FILE")
exec 2>&1


#########################################
# Flags
#########################################

USE_SUDO=true

for arg in "$@"; do
    case $arg in
        --rootuser)
            USE_SUDO=false
            shift
            ;;
        *)
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
# Start
#########################################

echo "================================="
echo " Debian Bootstrap Script Starting"
echo "================================="

#########################################
# System Update
#########################################

echo "==== Updating system ===="
run apt update
run apt upgrade -y

#########################################
# Core Packages
#########################################

echo "==== Installing core packages ===="

PACKAGES=(
    git
    curl
    unzip
)

# Install only if missing
for pkg in "${PACKAGES[@]}"; do
    if ! dpkg -s "$pkg" >/dev/null 2>&1; then
        echo "Installing $pkg..."
        run apt install -y "$pkg"
    else
        echo "$pkg already installed"
    fi
done

#########################################
# Install Latest Neovim (AppImage)
#########################################


echo "==== Installing Neovim (tarball) ===="

if ! command -v nvim &> /dev/null; then
    curl -LO https://github.com/neovim/neovim/releases/latest/download/nvim-linux-x86_64.tar.gz
    tar xzf nvim-linux-x86_64.tar.gz
    run cp -r nvim-linux-x86_64/* /usr/local/
    rm -rf nvim-linux-x86_64*
else
    echo "Neovim already installed"
fi


echo "Neovim version:"
nvim --version | head -n 1

# Using AppImage ensures latest stable instead of Debian's older package versions 【1-f7d5b1】

#########################################
# Neovim Config Setup
#########################################

echo "==== Setting up Neovim config ===="

NVIM_DIR="$HOME/.config/nvim"

if [ -d "$NVIM_DIR" ]; then
    echo "Existing config found — backing up..."
    mv "$NVIM_DIR" "$NVIM_DIR.bak.$(date +%s)"
fi

echo "Cloning Neovim config..."
git clone https://github.com/dowscope/Neovim-Configs.git "$NVIM_DIR"

#########################################
# Done
#########################################

echo "================================="
echo " Setup Complete!"
echo "================================="
echo ""
echo "Run: nvim"
echo "Logs saved to: $LOG_FILE"

