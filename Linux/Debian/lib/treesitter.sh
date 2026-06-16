install_treesitter() {
    log "Installing Treesitter..."

    if ! command_exists npm; then
        echo "npm not installed — skipping tree-sitter"
        return
    fi

    command_exists tree-sitter && return

    npm install -g tree-sitter-cli@0.24.7
}

remove_treesitter() {
    log "Removing Treesitter..."

    if ! command_exists npm; then
        echo "npm not installed — skipping remove"
        return
    fi

    npm uninstall -g tree-sitter-cli || true
}
