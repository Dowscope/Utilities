run() {
    if [[ "$(id -u)" -eq 0 ]]; then
        # already root → never use sudo
        "$@"
    elif command -v sudo >/dev/null 2>&1 && [[ "${USE_SUDO:-true}" == true ]]; then
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
    echo -e "\033[0;32m======================================"
    echo -e " $1"
    echo -e "======================================\033[0m"
}
