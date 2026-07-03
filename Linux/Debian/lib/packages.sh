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

    local found=()

    for pkg in "$@"; do
        if dpkg-query -W -f='${Status}' "$pkg" 2>/dev/null | grep -q "install ok"; then
            found+=("$pkg")
        fi
    done

    if [[ ${#found[@]} -eq 0 ]]; then
        echo "No packages to remove"
        return
    fi

    run apt-get purge -y "${found[@]}" || true
    run apt-get autoremove --purge -y || true
}
