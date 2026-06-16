#!/usr/bin/env bash

install_freeswitch() {

    echo "Installing FreeSWITCH..."


    if [[ ! -f /usr/share/keyrings/signalwire-freeswitch.gpg ]]; then

        echo "Adding FreeSWITCH signing key..."

        curl -s https://raw.githubusercontent.com/signalwire/freeswitch/master/docker/release/keys/signalwire-signing-key.pub \
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


    run systemctl enable freeswitch
    run systemctl restart freeswitch


    echo "FreeSWITCH installation complete."
}


remove_freeswitch() {

    echo "Removing FreeSWITCH..."

    run systemctl stop freeswitch || true
    run systemctl disable freeswitch || true

    run apt remove -y "${FREESWITCH_PACKAGES[@]}"

}
