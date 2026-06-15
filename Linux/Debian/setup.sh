#!/usr/bin/env bash

set -euo pipefail

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
MODE="install"

for arg in "$@"; do
    case "$arg" in
        --rootuser)
            USE_SUDO=false
            ;;
        --remove)
            MODE="remove"
            ;;
    esac
done

#########################################
# Global package definitions
#########################################

# Core packages required by the script itself
CORE_PACKAGES=(
    git
    curl
    unzip
    tar
)

# User-defined packages for your environment
# Add or remove packages here as needed
USERDEF_PACKAGES=(
    build-essential
    gcc
    make
    cmake
    nodejs
    npm
)

#########################################
# Global paths and URLs
#########################################

NVIM_TARBALL_URL="https://github.com/neovim/neovim/releases/latest/download/nvim-linux-x86_64.tar.gz"
NVIM_TARBALL_FILE="nvim-linux-x86_64.tar.gz"
NVIM_EXTRACT_DIR="nvim-linux-x86_64"

NVIM_CONFIG_REPO="https://github.com/dowscope/Neovim-Configs.git"
NVIM_CONFIG_DIR="$HOME/.config/nvim"

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

install_package_group() {
    local group_name="$1"
    shift
    local packages=("$@")

    echo "Installing ${group_name} packages"

    for pkg in "${packages[@]}"; do
        if ! dpkg -s "$pkg" >/dev/null 2>&1; then
            echo "Installing package: $pkg"
            run apt install -y "$pkg"
        else
            echo "Package already installed: $pkg"
        fi
    done
}

remove_package_group() {
    local group_name="$1"
    shift
    local packages=("$@")

    echo "Removing ${group_name} packages"

    for pkg in "${packages[@]}"; do
        if dpkg -s "$pkg" >/dev/null 2>&1; then
            echo "Removing package: $pkg"
            run apt remove -y "$pkg" || true
        else
            echo "Package not installed: $pkg"
        fi
    done
}

install_tree_sitter_cli() {
    echo "Installing tree-sitter CLI"

    if command_exists tree-sitter; then
        echo "tree-sitter CLI already installed"
    else
        run npm install -g tree-sitter-cli
    fi
}

remove_tree_sitter_cli() {
    echo "Removing tree-sitter CLI"

    if command_exists npm; then
        run npm uninstall -g tree-sitter-cli || true
    else
        echo "npm not found, skipping tree-sitter CLI removal"
    fi
}

install_neovim() {
    echo "Installing Neovim"

    if command_exists nvim; then
        echo "Neovim already installed"
        nvim --version | head -n 1 || true
        return
    fi

    rm -rf "$NVIM_EXTRACT_DIR" "$NVIM_TARBALL_FILE"

    curl -LO "$NVIM_TARBALL_URL"
    tar xzf "$NVIM_TARBALL_FILE"

    run cp -r "${NVIM_EXTRACT_DIR}/"* /usr/local/

    rm -rf "$NVIM_EXTRACT_DIR" "$NVIM_TARBALL_FILE"

    echo "Installed Neovim version:"
    nvim --version | head -n 1 || true
}

remove_neovim() {
    echo "Removing Neovim"

    run rm -f /usr/local/bin/nvim || true
    run rm -rf /usr/local/lib/nvim || true

    run rm -f /usr/local/share/applications/nvim.desktop || true
    run rm -f /usr/local/share/man/man1/nvim.1 || true

    run find /usr/local/share/icons -type f \( -name 'nvim.png' -o -name 'nvim.svg' \) -delete 2>/dev/null || true

    if command_exists nvim; then
        echo "Neovim is still present in PATH:"
        command -v nvim || true
    else
        echo "Neovim removed from PATH"
    fi
}

install_neovim_config() {
    echo "Setting up Neovim config"

    if [ -d "$NVIM_CONFIG_DIR" ]; then
        echo "Backing up existing config"
        mv "$NVIM_CONFIG_DIR" "${NVIM_CONFIG_DIR}.bak.$(date +%s)"
    fi

    git clone "$NVIM_CONFIG_REPO" "$NVIM_CONFIG_DIR"
}

remove_neovim_config() {
    echo "Removing Neovim config"

    if [ -d "$NVIM_CONFIG_DIR" ]; then
        rm -rf "$NVIM_CONFIG_DIR"
        echo "Removed config directory: $NVIM_CONFIG_DIR"
    else
        echo "No Neovim config directory found"
    fi
}

#########################################
# Install
#########################################

install() {
    echo "---------------------------------"
    echo "Starting install"
    echo "---------------------------------"

    echo "Updating system"
    run apt update
    run apt upgrade -y

    install_package_group "CORE" "${CORE_PACKAGES[@]}"
    install_package_group "USERDEF" "${USERDEF_PACKAGES[@]}"

    install_tree_sitter_cli
    install_neovim
    install_neovim_config

    echo "Install complete"
    echo "Log file: $LOG_FILE"
}

#########################################
# Remove
#########################################

remove() {
    echo "---------------------------------"
    echo "Starting remove"
    echo "---------------------------------"

    remove_tree_sitter_cli
    remove_neovim
    remove_neovim_config

    remove_package_group "USERDEF" "${USERDEF_PACKAGES[@]}"
    remove_package_group "CORE" "${CORE_PACKAGES[@]}"

    run apt autoremove -y || true

    echo "Remove complete"
    echo "Log file: $LOG_FILE"
}

#########################################
# Execute mode
#########################################

case "$MODE" in
    install)
        install
        ;;
    remove)
        remove
        ;;
    *)
        echo "Unknown mode: $MODE"
        exit 1
        ;;
esac

