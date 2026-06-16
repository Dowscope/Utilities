install_all() {

    log "INSTALL START"

    run apt update
    run apt upgrade -y

    install_packages "${CORE_PACKAGES[@]}"
    install_packages "${USER_PACKAGES[@]}"

    for module in "${MODULES[@]}"; do
        func="install_${module}"
        if declare -f "$func" >/dev/null; then
            "$func"
        else
            echo "Missing $func"
        fi
    done

    log "INSTALL COMPLETE"
}


remove_all() {

    log "REMOVE START"

    for (( idx=${#MODULES[@]}-1 ; idx>=0 ; idx-- )); do
        module="${MODULES[idx]}"
        func="remove_${module}"
        if declare -f "$func" >/dev/null; then
            "$func"
        else
            echo "Missing $func"
        fi
    done

    remove_packages "${USER_PACKAGES[@]}"

    log "REMOVE COMPLETE"
}
