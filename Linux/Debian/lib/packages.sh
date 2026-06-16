install_packages() {
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
    installed=()

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
