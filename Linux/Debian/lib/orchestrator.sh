install_all() {

    log "INSTALL START"

    run apt update
    run apt upgrade -y

    install_packages "${CORE_PACKAGES[@]}"
    install_packages "${USER_PACKAGES[@]}"

    install_node
    install_treesitter
    install_neovim

    log "INSTALL COMPLETE"
}

remove_all() {

    log "REMOVE START"

    remove_treesitter
    remove_node
    remove_neovim

    remove_packages "${USER_PACKAGES[@]}"

    log "REMOVE COMPLETE"
}
