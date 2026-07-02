#!/usr/bin/env bash

install_dev(){
    log "Setting up development environment..."

    install_dev_packages
    configure_git
    configure_github_ssh
}

remove_dev(){
    log "Removing development environment..."

    remove_github_ssh
}

install_dev_packages(){
    install_packages git openssh-client curl
}

configure_git(){
    log "Configuring Git..."

    git config --global user.name "$GIT_USER_NAME"
    git config --global user.email "$GIT_USER_EMAIL"
}

configure_github_ssh(){
    log "Configuring GitHub SSH key..."

    mkdir -p "$HOME/.ssh"
    chmod 700 "$HOME/.ssh"

    local key_path="$HOME/.ssh/$GITHUB_SSH_KEY_NAME"

    if [[ -f "$key_path" ]]; then
        log "GitHub SSH key already exists."
    elif [[ -n "${GITHUB_SSH_KEY_SOURCE:-}" ]]; then
        if [[ ! -f "$GITHUB_SSH_KEY_SOURCE" ]]; then
            echo "SSH key source not found: $GITHUB_SSH_KEY_SOURCE"
            exit 1
        fi

        cp "$GITHUB_SSH_KEY_SOURCE" "$key_path"
        chmod 600 "$key_path"

        if [[ -f "$GITHUB_SSH_KEY_SOURCE.pub" ]]; then
            cp "$GITHUB_SSH_KEY_SOURCE.pub" "$key_path.pub"
            chmod 644 "$key_path.pub"
        else
            ssh-keygen -y -f "$key_path" > "$key_path.pub"
            chmod 644 "$key_path.pub"
        fi

        log "GitHub SSH key copied from source."
    else
        ssh-keygen -t ed25519 -C "$GIT_USER_EMAIL" -f "$key_path" -N ""
        log "GitHub SSH key generated."
    fi

    touch "$HOME/.ssh/config"

    if ! grep -q "IdentityFile ~/.ssh/$GITHUB_SSH_KEY_NAME" "$HOME/.ssh/config"; then
        cat >> "$HOME/.ssh/config" <<EOF

Host github.com
    HostName github.com
    User git
    IdentityFile ~/.ssh/$GITHUB_SSH_KEY_NAME
    IdentitiesOnly yes
EOF
    fi

    chmod 600 "$HOME/.ssh/config"

    echo
    echo "GitHub SSH public key:"
    echo
    cat "$key_path.pub"
    echo
}

remove_github_ssh(){
    local key_path="$HOME/.ssh/$GITHUB_SSH_KEY_NAME"
    local source_path="${GITHUB_SSH_KEY_SOURCE:-}"

    if [[ ! -f "$key_path" ]]; then
        log "No GitHub SSH key found to remove."
        return
    fi

    if [[ -n "$source_path" && -f "$source_path" ]]; then
        log "SSH key source already exists: $source_path"
    elif [[ -n "$source_path" && ! -f "$source_path" ]]; then
        echo
        echo "SSH key source does not exist:"
        echo "$source_path"
        echo
        read -rp "Save current GitHub SSH key to this source location before removing? [y/N]: " answer

        if [[ "$answer" =~ ^[Yy]$ ]]; then
            mkdir -p "$(dirname "$source_path")"
            cp "$key_path" "$source_path"
            chmod 600 "$source_path"

            if [[ -f "$key_path.pub" ]]; then
                cp "$key_path.pub" "$source_path.pub"
                chmod 644 "$source_path.pub"
            fi

            log "GitHub SSH key saved to source location."
        fi
    fi

    rm -f "$key_path" "$key_path.pub"

    log "GitHub SSH key removed."
}
