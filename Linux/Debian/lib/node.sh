install_node() {

    if command_exists node; then
        echo "Node already installed: $(node -v)"
        return
    fi

    log "Installing Node.js (LTS)"

    # install repo setup WITHOUT sudo
    curl -fsSL https://deb.nodesource.com/setup_lts.x | run bash -

    run apt install -y nodejs

    echo "Node: $(node -v)"
    echo "npm:  $(npm -v)"
}

remove_node() {

    log "Removing Node.js"

    if command_exists apt; then
        run apt remove -y nodejs || true
        run apt autoremove -y || true
    fi
}
