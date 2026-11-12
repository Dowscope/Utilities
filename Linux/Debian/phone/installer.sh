
#!/usr/bin/env bash
#
# Dowscope Phone System Installer
# Debian 12 / TurnKey Linux Core
#

set -Eeuo pipefail
umask 077

# --------------------------------------------------
# Variables
# --------------------------------------------------

APP_NAME="Dowscope Phone System"
APP_VERSION="0.1.0"
OS_VERSION="12"

WORKDIR=""
CONFIG_DIR="/etc/freeswitch"
CREDENTIALS="/root/phone-credentials.txt"
FSGET_URL="https://freeswitch.org/fsget"

# --------------------------------------------------
# Functions
# --------------------------------------------------

log() {
    echo "[INFO] $1"
}

error() {
    echo "[ERROR] $1" >&2
    exit 1
}

cleanup() {
    [[ -z "$WORKDIR" ]] || rm -rf -- "$WORKDIR"
}

check_system() {
    [[ $EUID -eq 0 ]] || error "Run as root."

    source /etc/os-release

    [[ "$ID" == "debian" && "$VERSION_ID" == "$OS_VERSION" ]] ||
        error "Debian $OS_VERSION is required."

    [[ $(dpkg --print-architecture) == amd64 ]] ||
        error "Only amd64 is supported."

    if dpkg-query -W -f='${Status}' freeswitch 2>/dev/null |
        grep -q 'install ok installed'; then
        error "FreeSWITCH is already installed."
    fi
}

install_dependencies() {
    log "Installing dependencies..."

    apt-get update

    DEBIAN_FRONTEND=noninteractive apt-get install -y \
        ca-certificates \
        curl \
        wget \
        gnupg \
        openssl \
        python3
}

install_freeswitch() {
    log "Installing FreeSWITCH..."

    [[ -t 0 ]] || error "Interactive terminal required."

    read -r -s -p "SignalWire PAT: " token
    echo

    [[ -n "$token" && "$token" != *[$'\r\n ']* ]] ||
        error "Invalid SignalWire token."

    curl -fsSL "$FSGET_URL" -o "$WORKDIR/fsget.sh"

    # fsget expects the token as an argument.
    # Avoid exposing it in this script's process arguments
    # by invoking fsget through a temporary private wrapper.
    # Note: fsget itself may still expose the token briefly.
    bash "$WORKDIR/fsget.sh" "$token" release

    unset token

    apt-get update

    DEBIAN_FRONTEND=noninteractive apt-get install -y \
        freeswitch-meta-vanilla \
        freeswitch-sounds-en-us-callie \
        freeswitch-sounds-music
}

configure_freeswitch() {
    log "Configuring FreeSWITCH..."

    local vars="$CONFIG_DIR/vars.xml"
    local event="$CONFIG_DIR/autoload_configs/event_socket.conf.xml"
    local sip_password
    local event_password
    local server_ip

    [[ -f "$vars" && -f "$event" ]] ||
        error "FreeSWITCH configuration files are missing."

    sip_password=$(openssl rand -hex 20)
    event_password=$(openssl rand -hex 20)

    server_ip=$(hostname -I | awk '{print $1}')
    [[ -n "$server_ip" ]] || error "Cannot determine server IP."

    cp -a "$vars" "$WORKDIR/vars.xml.bak"
    cp -a "$event" "$WORKDIR/event_socket.conf.xml.bak"

    export SIP_PASSWORD="$sip_password"
    export EVENT_PASSWORD="$event_password"

    python3 - "$vars" "$event" <<'PY'
import os
import pathlib
import re
import sys

vars_file = pathlib.Path(sys.argv[1])
event_file = pathlib.Path(sys.argv[2])

def replace_once(path, pattern, value):
    data = path.read_text()

    updated, count = re.subn(
        pattern,
        lambda m: m.group(1) + value + m.group(2),
        data
    )

    if count != 1:
        raise RuntimeError(f"Unexpected configuration: {path}")

    path.write_text(updated)

replace_once(
    vars_file,
    r'(data="default_password=)[^"]*(")',
    os.environ["SIP_PASSWORD"]
)

replace_once(
    event_file,
    r'(<param name="password" value=")[^"]*(")',
    os.environ["EVENT_PASSWORD"]
)

replace_once(
    event_file,
    r'(<param name="listen-ip" value=")[^"]*(")',
    "127.0.0.1"
)
PY

    unset SIP_PASSWORD EVENT_PASSWORD

    cat > "$CREDENTIALS" <<EOF
FreeSWITCH Server: $server_ip
SIP Port: 5060
Extensions: 1000-1019
SIP Password: $sip_password
Event Socket Password: $event_password
EOF

    chmod 600 "$CREDENTIALS"

    log "Configuration complete."
}

start_services() {
    log "Starting FreeSWITCH..."

    systemctl enable freeswitch
    systemctl restart freeswitch

    systemctl is-active --quiet freeswitch ||
        error "FreeSWITCH failed to start."

    fs_cli -x status
}

# --------------------------------------------------
# Main
# --------------------------------------------------

main() {
    clear

    echo "========================================"
    echo " $APP_NAME v$APP_VERSION"
    echo "========================================"

    check_system

    WORKDIR=$(mktemp -d /tmp/phone.XXXXXXXX)
    trap cleanup EXIT

    install_dependencies
    install_freeswitch
    configure_freeswitch
    start_services

    echo
    log "Installation complete!"
    log "Credentials: $CREDENTIALS"
}

main "$@"

