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
        return 0
    fi

    local group="$1"
    shift

    log "Removing $group"

    if [[ $# -eq 0 ]]; then
        echo "No packages to remove"
        return 0
    fi

    local found=()
    local pkg
    local status

    for pkg in "$@"; do
        status="$(dpkg-query -W -f='${db:Status-Abbrev}' "$pkg" 2>/dev/null || true)"

        case "$status" in
            ii*|rc*)
                found+=("$pkg")
                ;;
        esac
    done

    if [[ ${#found[@]} -eq 0 ]]; then
        echo "No packages to remove"
        return 0
    fi

    echo "Purging packages:"
    printf '  %s\n' "${found[@]}"

    run apt-get purge -y "${found[@]}" || {
        echo "Package purge failed"
        return 1
    }

    run apt-get autoremove --purge -y || {
        echo "Package autoremove failed"
        return 1
    }
}
