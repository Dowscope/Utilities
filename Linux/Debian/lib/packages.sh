install_packages() {
    local group="$1"
    shift

    log "Installing $group"

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
    local group="$1"
    shift

    log "Removing $group"

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

    for pkg in "${installed[@]}"; do
        echo "Removing $pkg..."
    done

    run apt remove -y "${installed[@]}" || true
    run apt autoremove -y || true
}
