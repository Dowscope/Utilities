install_treesitter() {

    if command_exists tree-sitter; then
        echo "Tree-sitter already installed"
        return
    fi

    log "Installing Tree-sitter CLI"

    npm install -g tree-sitter-cli

    echo "tree-sitter: $(tree-sitter --version)"
}

remove_treesitter() {

    log "Removing Tree-sitter CLI"

    npm uninstall -g tree-sitter-cli || true
}
