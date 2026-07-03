NEOVIM_CONFIG_URL="https://github.com/Dowscope/NeoVim-Configs/archive/refs/heads/main.tar.gz"

install_neovim_config() {
    log "Installing Neovim config"

    local config_tmp="$RUN_TMP/neovim-config"

    rm -rf "$config_tmp"
    mkdir -p "$config_tmp"

    curl -fsSL "$NEOVIM_CONFIG_URL" -o "$config_tmp/config.tar.gz"
    tar -xzf "$config_tmp/config.tar.gz" -C "$config_tmp"

    rm -rf "$HOME/.config/nvim"
    mkdir -p "$HOME/.config"
    cp -a "$config_tmp/Neovim-Configs-main/." "$HOME/.config/nvim"

    echo "Neovim config installed to $HOME/.config/nvim"
}

remove_neovim_config() {
    log "Removing Neovim config"

    rm -rf "$HOME/.config/nvim"
}

install_neovim() {
    log "Installing Neovim"

    if command_exists nvim; then
        echo "Neovim already installed"
    else
        local archive="nvim-linux-x86_64.tar.gz"
        local folder="nvim-linux-x86_64"

        curl -LO https://github.com/neovim/neovim/releases/latest/download/${archive}
        tar xzf "$archive"

        run cp -a "${folder}/." /usr/local/

        rm -rf "$archive" "$folder"

        echo "Neovim: $(nvim --version | head -n 1)"
    fi

    install_neovim_config
}

remove_neovim() {
    log "Removing Neovim"

    run rm -f /usr/local/bin/nvim
    run rm -rf /usr/local/share/nvim
    run rm -rf /usr/local/lib/nvim

    remove_neovim_config
}

