########################################
# FreeSWITCH Configuration
########################################

FREESWITCH_PACKAGES=(
  jq
  gettext-base
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
FREESWITCH_SERVICE_URL="$REPO_BASE/config/freeswitch.service"
FREESWITCH_SERVICE_TARGET="/etc/systemd/system/$FREESWITCH_SERVICE"
FREESWITCH_CONFIG_BASE="$REPO_BASE/configs/freeswitch"
FREESWITCH_CONFIG_TMP="$RUN_TMP/configs/freeswitch"
FREESWITCH_CONFIG_MANIFEST="$FREESWITCH_CONFIG_TMP/manifest.json"

########################################
# Install
########################################

install_freeswitch(){
    log "Installing FreeSWITCH..."

    if dpkg-query -W -f='${Status}' freeswitch 2>/dev/null | grep -q "install ok installed"; then
        echo "FreeSWITCH already installed"
        configure_freeswitch
        download_freeswitch_configs
        deploy_freeswitch_configs
        install_freeswitch_service
        return
    fi

    if [[ -z "${SIGNALWIRE_TOKEN:-}" ]]; then
        echo "SIGNALWIRE_TOKEN is required for FreeSWITCH"
        return 1
    fi

    if [[ ! -s "$FREESWITCH_KEYRING" ]]; then
        echo "Adding FreeSWITCH signing key..."
        curl -u "signalwire:${SIGNALWIRE_TOKEN}" -fsSL "$FREESWITCH_KEY_URL" -o /tmp/signalwire-freeswitch.gpg || {
            echo "Failed to download FreeSWITCH signing key"
            return 1
        }
        run mv /tmp/signalwire-freeswitch.gpg "$FREESWITCH_KEYRING"
    fi

    echo "Configuring FreeSWITCH authentication..."
    run mkdir -p /etc/apt/auth.conf.d
    echo "machine freeswitch.signalwire.com login signalwire password ${SIGNALWIRE_TOKEN}" | run tee "$FREESWITCH_AUTH_FILE" >/dev/null
    run chmod 600 "$FREESWITCH_AUTH_FILE"

    echo "Configuring FreeSWITCH repository..."
    echo "deb [signed-by=$FREESWITCH_KEYRING] $FREESWITCH_REPO_URL $FREESWITCH_REPO_DIST $FREESWITCH_REPO_COMPONENT" | run tee "$FREESWITCH_REPO_FILE" >/dev/null

    run apt update || return 1

    echo "Installing FreeSWITCH packages..."
    run apt install -y "${FREESWITCH_PACKAGES[@]}"

    configure_freeswitch
    download_freeswitch_configs
    deploy_freeswitch_configs
    install_freeswitch_service

    echo "FreeSWITCH installation complete."
}

########################################
# Configure
########################################

configure_freeswitch(){
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

download_freeswitch_configs(){
    echo "Downloading FreeSWITCH configs..."

    run rm -rf "$FREESWITCH_CONFIG_TMP"
    run mkdir -p "$FREESWITCH_CONFIG_TMP"

    curl -fsSL "$FREESWITCH_CONFIG_BASE/manifest.json" -o "$FREESWITCH_CONFIG_MANIFEST" || {
        echo "Failed to download FreeSWITCH config manifest"
        return 1
    }

    jq -c '.[]' "$FREESWITCH_CONFIG_MANIFEST" | while read -r item; do
        local source
        source=$(echo "$item" | jq -r '.source')
        run mkdir -p "$FREESWITCH_CONFIG_TMP/$(dirname "$source")"
        curl -fsSL "$FREESWITCH_CONFIG_BASE/$source" -o "$FREESWITCH_CONFIG_TMP/$source" || {
            echo "Failed to download FreeSWITCH config: $source"
            return 1
        }
    done
}

deploy_freeswitch_configs(){
    echo "Deploying FreeSWITCH configs..."

    jq -c '.[]' "$FREESWITCH_CONFIG_MANIFEST" | while read -r item; do
        local source target mode
        source=$(echo "$item" | jq -r '.source')
        target=$(echo "$item" | jq -r '.target')
        mode=$(echo "$item" | jq -r '.mode')

        run mkdir -p "$(dirname "$target")"

        if [[ "$mode" == "template" ]]; then
            envsubst < "$FREESWITCH_CONFIG_TMP/$source" | run tee "$target" >/dev/null
        else
            run cp "$FREESWITCH_CONFIG_TMP/$source" "$target"
        fi

        run chown freeswitch:freeswitch "$target"
    done
}

install_freeswitch_service(){
    echo "Installing FreeSWITCH service..."

    curl -fsSL "$FREESWITCH_SERVICE_URL" -o /tmp/$FREESWITCH_SERVICE || {
        echo "Failed to download FreeSWITCH service"
        return 1
    }

    run mv /tmp/$FREESWITCH_SERVICE "$FREESWITCH_SERVICE_TARGET"
    run systemctl daemon-reload
    run systemctl enable "$FREESWITCH_SERVICE"
    run systemctl restart "$FREESWITCH_SERVICE"
}

########################################
# Remove
########################################

remove_freeswitch(){
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

    echo "Removing FreeSWITCH service..."

    if [[ -f "/etc/systemd/system/$FREESWITCH_SERVICE" ]]; then
        run systemctl stop "$FREESWITCH_SERVICE" || true
        run systemctl disable "$FREESWITCH_SERVICE" || true
        run rm -f "/etc/systemd/system/$FREESWITCH_SERVICE"
        run systemctl daemon-reload
        run systemctl reset-failed || true
    fi

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
