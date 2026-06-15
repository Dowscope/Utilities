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
    for pkg in "$@"; do
        if dpkg -s "$pkg" >/dev/null 2>&1; then
            echo "Removing $pkg..."
            run apt remove -y "$pkg"
        fi
    done

    run apt autoremove -y
}
