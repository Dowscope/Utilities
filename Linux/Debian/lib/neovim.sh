install_neovim() {
    log "Installing Neovim"

    if command_exists nvim; then
        echo "Neovim already installed"
        return
    fi

    local archive="nvim-linux-x86_64.tar.gz"
    local folder="nvim-linux-x86_64"

    curl -LO https://github.com/neovim/neovim/releases/latest/download/${archive}
    tar xzf "$archive"

    run cp -a "${folder}/." /usr/local/

    rm -rf "$archive" "$folder"

    echo "Neovim: $(nvim --version | head -n 1)"
}

remove_neovim() {
    log "Removing Neovim"

    run rm -f /usr/local/bin/nvim
    run rm -rf /usr/local/share/nvim
    run rm -rf /usr/local/lib/nvim
}
