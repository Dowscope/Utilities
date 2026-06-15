run() {
    if [[ "${USE_SUDO:-true}" == true ]]; then
        sudo "$@"
    else
        "$@"
    fi
}

command_exists() {
    command -v "$1" >/dev/null 2>&1
}

log() {
    echo
    echo "======================================"
    echo " $1"
    echo "======================================"
}
