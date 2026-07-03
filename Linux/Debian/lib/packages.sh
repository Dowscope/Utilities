install_packages() {
    if [[ $# -eq 0 ]]; then
        return
    fi

    local group="$1"
    shift

    log "Installing $group"

    if [[ $# -eq 0 ]]; then
        echo "No packages to install"
        return
    fi

    for pkg in "$@"; do
        if dpkg -s "$pkg" >/dev/null 2>&1; then
            echo "$pkg already installed"
        else
            echo "Installing $pkg..."
            run apt install -y "$pkg"
        fi
    done
}

remove_packages() {
    if [[ $# -eq 0 ]]; then
        return
    fi

    local group="$1"
    shift

    log "Removing $group"

    if [[ $# -eq 0 ]]; then
        echo "No packages to remove"
        return
    fi

    local installed=()

    for pkg in "$@"; do
        if dpkg -s "$pkg" >/dev/null 2>&1; then
            installed+=("$pkg")
        fi
    done

    if [[ ${#installed[@]} -eq 0 ]]; then
        echo "No packages to remove"
        return
    fi

    run apt remove -y "${installed[@]}" || true
    run apt autoremove -y || true
}
