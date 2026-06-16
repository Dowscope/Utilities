#!/usr/bin/env bash

########################################
# FreeSWITCH Configuration
########################################

FREESWITCH_KEYRING="/usr/share/keyrings/signalwire-freeswitch.gpg"
FREESWITCH_REPO_FILE="/etc/apt/sources.list.d/freeswitch.list"
FREESWITCH_REPO_URL="https://freeswitch.signalwire.com/repo/deb/debian-release/"
FREESWITCH_REPO_DIST="bookworm"
FREESWITCH_REPO_COMPONENT="main"
FREESWITCH_KEY_URL="https://freeswitch.signalwire.com/repo/deb/debian-release/signalwire-freeswitch-repo.gpg"
FREESWITCH_AUTH_FILE="/etc/apt/auth.conf.d/freeswitch.conf"
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

    if [[ -z "${SIGNALWIRE_TOKEN:-}" ]]; then
        echo "SIGNALWIRE_TOKEN is required for FreeSWITCH"
        return 1
    fi

    if [[ ! -s "$FREESWITCH_KEYRING" ]]; then
        echo "Adding FreeSWITCH signing key..."

        curl -u "signalwire:${SIGNALWIRE_TOKEN}" \
        -fsSL "$FREESWITCH_KEY_URL" \
        | run tee "$FREESWITCH_KEYRING" >/dev/null
    fi

    echo "Configuring FreeSWITCH repository..."

    echo "machine freeswitch.signalwire.com login signalwire password ${SIGNALWIRE_TOKEN}" \
    | run tee "$FREESWITCH_AUTH_FILE" >/dev/null

    run chmod 600 "$FREESWITCH_AUTH_FILE"

    echo "deb [signed-by=$FREESWITCH_KEYRING] $FREESWITCH_REPO_URL $FREESWITCH_REPO_DIST $FREESWITCH_REPO_COMPONENT" \
    | run tee "$FREESWITCH_REPO_FILE" >/dev/null

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
        run systemctl stop "$FREESWITCH_SERVICE" || true
        run systemctl disable "$FREESWITCH_SERVICE" || true

        echo "Removing packages..."

        run apt remove -y "${installed[@]}" || true
        run apt autoremove -y || true
    else
        echo "FreeSWITCH packages not installed"
    fi

    if [[ -f "$FREESWITCH_REPO_FILE" ]]; then
        echo "Removing FreeSWITCH repository..."
        run rm -f "$FREESWITCH_REPO_FILE"
    fi

    if [[ -f "$FREESWITCH_AUTH_FILE" ]]; then
        echo "Removing FreeSWITCH authentication..."
        run rm -f "$FREESWITCH_AUTH_FILE"
    fi

    if [[ -f "$FREESWITCH_KEYRING" ]]; then
        echo "Removing FreeSWITCH signing key..."
        run rm -f "$FREESWITCH_KEYRING"
    fi

    echo "FreeSWITCH cleanup complete."
}
