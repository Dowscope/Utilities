install_freeswitch() {
    log "Installing FreeSWITCH..."

    installed=false

    for pkg in "${FREESWITCH_PACKAGES[@]}"; do
        if dpkg -s "$pkg" >/dev/null 2>&1; then
            installed=true
            break
        fi
    done

    if [[ "$installed" == true ]]; then
        echo "FreeSWITCH already installed"
        return
    fi

    if [[ ! -f /usr/share/keyrings/signalwire-freeswitch.gpg ]]; then
        echo "Adding FreeSWITCH signing key..."
        curl -fsSL https://raw.githubusercontent.com/signalwire/freeswitch/master/docker/release/keys/signalwire-signing-key.pub \
        | gpg --dearmor \
        | run tee /usr/share/keyrings/signalwire-freeswitch.gpg >/dev/null
    fi

    if [[ ! -f /etc/apt/sources.list.d/freeswitch.list ]]; then
        echo "Adding FreeSWITCH repository..."
        echo "deb [signed-by=/usr/share/keyrings/signalwire-freeswitch.gpg] http://deb.freeswitch.org/repo/deb/debian-release/ bookworm main" \
        | run tee /etc/apt/sources.list.d/freeswitch.list >/dev/null
    fi

    run apt update

    echo "Installing FreeSWITCH packages..."
    run apt install -y "${FREESWITCH_PACKAGES[@]}"

    if systemctl list-unit-files | grep -q freeswitch.service; then
        run systemctl enable freeswitch
        run systemctl restart freeswitch
    fi

    echo "FreeSWITCH installation complete."
}


remove_freeswitch() {
    log "Removing FreeSWITCH..."

    installed=()

    for pkg in "${FREESWITCH_PACKAGES[@]}"; do
        if dpkg -s "$pkg" >/dev/null 2>&1; then
            installed+=("$pkg")
        fi
    done

    if [[ ${#installed[@]} -eq 0 ]]; then
        echo "FreeSWITCH not installed — skipping"
        return
    fi

    if systemctl list-unit-files | grep -q freeswitch.service; then
        run systemctl stop freeswitch || true
        run systemctl disable freeswitch || true
    fi

    echo "Removing packages..."
    run apt remove -y "${installed[@]}" || true
    run apt autoremove -y || true

    echo "FreeSWITCH removed."
}
