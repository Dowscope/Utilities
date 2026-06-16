#!/usr/bin/env bash

########################################
# FreeSWITCH Configuration
########################################

FREESWITCH_KEYRING="/usr/share/keyrings/signalwire-freeswitch.gpg"
FREESWITCH_REPO_FILE="/etc/apt/sources.list.d/freeswitch.list"
FREESWITCH_REPO_URL="http://deb.freeswitch.org/repo/deb/debian-release/"
FREESWITCH_REPO_DIST="bookworm"
FREESWITCH_REPO_COMPONENT="main"
FREESWITCH_KEY_URL="https://freeswitch.signalwire.com/repo/deb/debian-release/signalwire-freeswitch-repo.gpg"
FREESWITCH_SERVICE="freeswitch.service"

########################################
# Install
########################################

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

    if [[ ! -s "$FREESWITCH_KEYRING" ]]; then
        echo "Adding FreeSWITCH signing key..."
        curl -fsSL "$FREESWITCH_KEY_URL" | run tee "$FREESWITCH_KEYRING" >/dev/null
    fi

    if [[ ! -f "$FREESWITCH_REPO_FILE" ]]; then
        echo "Adding FreeSWITCH repository..."

        echo "deb [signed-by=$FREESWITCH_KEYRING] $FREESWITCH_REPO_URL $FREESWITCH_REPO_DIST $FREESWITCH_REPO_COMPONENT" \
        | run tee "$FREESWITCH_REPO_FILE" >/dev/null
    fi

    run apt update

    echo "Installing FreeSWITCH packages..."
    run apt install -y "${FREESWITCH_PACKAGES[@]}"

    if systemctl list-unit-files | grep -q "$FREESWITCH_SERVICE"; then
        run systemctl enable "$FREESWITCH_SERVICE"
        run systemctl restart "$FREESWITCH_SERVICE"
    fi

    echo "FreeSWITCH installation complete."
}

########################################
# Remove
########################################

remove_freeswitch() {
    log "Removing FreeSWITCH"

    installed=()

    for pkg in "${FREESWITCH_PACKAGES[@]}"; do
        if dpkg -s "$pkg" >/dev/null 2>&1; then
            installed+=("$pkg")
        fi
    done

    if [[ ${#installed[@]} -gt 0 ]]; then
        run systemctl stop freeswitch || true
        run systemctl disable freeswitch || true

        echo "Removing packages..."

        run apt remove -y "${installed[@]}" || true
        run apt autoremove -y || true
    else
        echo "FreeSWITCH packages not installed"
    fi

    if [[ -f /etc/apt/sources.list.d/freeswitch.list ]]; then
        echo "Removing FreeSWITCH repository..."
        run rm -f /etc/apt/sources.list.d/freeswitch.list
    fi

    if [[ -f /usr/share/keyrings/signalwire-freeswitch.gpg ]]; then
        echo "Removing FreeSWITCH signing key..."
        run rm -f /usr/share/keyrings/signalwire-freeswitch.gpg
    fi

    echo "FreeSWITCH cleanup complete."
}
