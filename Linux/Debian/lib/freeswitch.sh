########################################
# FreeSWITCH Configuration
########################################

FREESWITCH_PACKAGES=(
  freeswitch
  freeswitch-conf-vanilla
  freeswitch-mod-sofia
  freeswitch-mod-conference
  freeswitch-mod-event-socket
  freeswitch-mod-commands
  freeswitch-mod-db
  freeswitch-mod-console
  freeswitch-sounds-en-us-callie
  freeswitch-music-default
)

FREESWITCH_KEYRING="/usr/share/keyrings/signalwire-freeswitch.gpg"
FREESWITCH_REPO_FILE="/etc/apt/sources.list.d/freeswitch.list"
FREESWITCH_REPO_URL="https://freeswitch.signalwire.com/repo/deb/debian-release/"
FREESWITCH_REPO_DIST="$DEBIAN_CODENAME"
FREESWITCH_REPO_COMPONENT="main"
FREESWITCH_KEY_URL="https://freeswitch.signalwire.com/repo/deb/debian-release/signalwire-freeswitch-repo.gpg"
FREESWITCH_AUTH_FILE="/etc/apt/auth.conf.d/freeswitch.conf"
FREESWITCH_SERVICE="freeswitch.service"
FREESWITCH_SOURCE_CONF="/usr/share/freeswitch/conf/vanilla"
FREESWITCH_TARGET_CONF="/etc/freeswitch"

########################################
# Install
########################################

install_freeswitch() {
    log "Installing FreeSWITCH..."

    installed=false

    for pkg in "${FREESWITCH_PACKAGES[@]}"; do
        if dpkg-query -W -f='${Status}' "$pkg" 2>/dev/null | grep -q "install ok installed"; then
            installed=true
            break
        fi
    done

    if [[ "$installed" == true ]]; then
        echo "FreeSWITCH already installed"
        configure_freeswitch
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
        -o /tmp/signalwire-freeswitch.gpg || {
            echo "Failed to download FreeSWITCH signing key"
            return 1
        }

        run mv /tmp/signalwire-freeswitch.gpg "$FREESWITCH_KEYRING"
    fi

    echo "Configuring FreeSWITCH authentication..."

    run mkdir -p /etc/apt/auth.conf.d

    echo "machine freeswitch.signalwire.com login signalwire password ${SIGNALWIRE_TOKEN}" \
    | run tee "$FREESWITCH_AUTH_FILE" >/dev/null

    run chmod 600 "$FREESWITCH_AUTH_FILE"

    echo "Configuring FreeSWITCH repository..."

    echo "deb [signed-by=$FREESWITCH_KEYRING] $FREESWITCH_REPO_URL $FREESWITCH_REPO_DIST $FREESWITCH_REPO_COMPONENT" \
    | run tee "$FREESWITCH_REPO_FILE" >/dev/null

    run apt update || return 1

    echo "Installing FreeSWITCH packages..."

    run apt install -y "${FREESWITCH_PACKAGES[@]}"

    configure_freeswitch

    if systemctl list-unit-files | grep -q "$FREESWITCH_SERVICE"; then
        run systemctl enable "$FREESWITCH_SERVICE"
        run systemctl restart "$FREESWITCH_SERVICE"
    fi

    echo "FreeSWITCH installation complete."
}

########################################
# Configure
########################################

configure_freeswitch() {
    echo "Configuring FreeSWITCH files..."

    if [[ ! -f "$FREESWITCH_TARGET_CONF/freeswitch.xml" ]]; then
        echo "Copying vanilla FreeSWITCH configuration..."

        run mkdir -p "$FREESWITCH_TARGET_CONF"
        run cp -a "$FREESWITCH_SOURCE_CONF"/* "$FREESWITCH_TARGET_CONF"/
    else
        echo "FreeSWITCH configuration already exists"
    fi

    run chown -R freeswitch:freeswitch "$FREESWITCH_TARGET_CONF"
}

########################################
# Remove
########################################

remove_freeswitch() {
    log "Removing FreeSWITCH"

    run systemctl stop "$FREESWITCH_SERVICE" || true
    run systemctl disable "$FREESWITCH_SERVICE" || true

    installed=()

    for pkg in "${FREESWITCH_PACKAGES[@]}"; do
        if dpkg-query -W -f='${Status}' "$pkg" 2>/dev/null | grep -q "install ok installed"; then
            installed+=("$pkg")
        fi
    done

    if [[ ${#installed[@]} -gt 0 ]]; then
        echo "Removing packages..."
        run apt purge -y "${installed[@]}" || true
    else
        echo "No FreeSWITCH packages installed"
    fi

    run apt autoremove -y || true

    if [[ -d "$FREESWITCH_TARGET_CONF" ]]; then
        echo "Removing FreeSWITCH configuration..."
        run rm -rf "$FREESWITCH_TARGET_CONF"
    fi

    if [[ -f "$FREESWITCH_REPO_FILE" ]]; then
        run rm -f "$FREESWITCH_REPO_FILE"
    fi

    if [[ -f "$FREESWITCH_AUTH_FILE" ]]; then
        run rm -f "$FREESWITCH_AUTH_FILE"
    fi

    if [[ -f "$FREESWITCH_KEYRING" ]]; then
        run rm -f "$FREESWITCH_KEYRING"
    fi

    echo "FreeSWITCH cleanup complete."
}
