install_treesitter() {

    if ! command_exists npm; then
        echo "npm not installed — skipping tree-sitter"
        return
    fi

    command_exists tree-sitter && return

    npm install -g tree-sitter-cli@0.24.7
}

remove_treesitter() {

    if ! command_exists npm; then
        echo "npm not installed — skipping remove"
        return
    fi

    npm uninstall -g tree-sitter-cli || true
}
