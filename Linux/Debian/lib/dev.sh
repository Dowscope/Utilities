#!/usr/bin/env bash

########################################
# Development Configuration
########################################

DEV_PACKAGES=(
  git
  openssh-client
  curl
)

DOTNET_DEV_PACKAGES=(
  dotnet-sdk-8.0
)

DEV_STATE_DIR="$DOWSCOPE_STATE_DIR/dev"
DEV_GIT_BACKUP="$DEV_STATE_DIR/git.conf"
DEV_SSH_DIR="$HOME/.ssh"
DEV_SSH_CONFIG="$DEV_SSH_DIR/config"

########################################
# Install
########################################

install_dev(){
    log "Setting up development environment..."

    if [[ -z "${GIT_USER_NAME:-}" ]]; then
        echo "GIT_USER_NAME is required for development"
        return 1
    fi

    if [[ -z "${GIT_USER_EMAIL:-}" ]]; then
        echo "GIT_USER_EMAIL is required for development"
        return 1
    fi

    if [[ -z "${GITHUB_SSH_KEY_NAME:-}" ]]; then
        echo "GITHUB_SSH_KEY_NAME is required for development"
        return 1
    fi

    install_dev_packages

    log "Configuring Dev..."
    configure_git
    configure_github_ssh

    echo "Development environment setup complete."
}

install_dev_packages(){
    echo "Installing development packages..."
    install_packages "${DEV_PACKAGES[@]}"
}

install_dev_dotnet() {
  if [ "$ENV_DOTNET" != true ]; then
    return
  fi

  for package in "${DOTNET_DEV_PACKAGES[@]}"; do
    echo "Installing $package..."
    run apt-get install -y "$package"
  done
}

########################################
# Configure
########################################

configure_git(){
    echo "Configuring Git..."

    local current_name
    local current_email

    current_name="$(git config --global --get user.name || true)"
    current_email="$(git config --global --get user.email || true)"

    if [[ "$current_name" == "$GIT_USER_NAME" && "$current_email" == "$GIT_USER_EMAIL" ]]; then
        echo "Git already configured."
        return
    fi

    backup_git

    git config --global user.name "$GIT_USER_NAME"
    git config --global user.email "$GIT_USER_EMAIL"
}

backup_git(){
    if [[ -f "$DEV_GIT_BACKUP" ]]; then
        echo "Git backup already exists."
        return
    fi

    mkdir -p "$DEV_STATE_DIR"
    : > "$DEV_GIT_BACKUP"

    if git config --global --get user.name >/dev/null 2>&1; then
        echo "user.name=$(git config --global --get user.name)" >> "$DEV_GIT_BACKUP"
    fi

    if git config --global --get user.email >/dev/null 2>&1; then
        echo "user.email=$(git config --global --get user.email)" >> "$DEV_GIT_BACKUP"
    fi
}

configure_github_ssh(){
    echo "Configuring GitHub SSH key..."

    mkdir -p "$DEV_SSH_DIR"
    chmod 700 "$DEV_SSH_DIR"

    local key_path="$DEV_SSH_DIR/$GITHUB_SSH_KEY_NAME"

    if [[ -f "$key_path" ]]; then
        echo "GitHub SSH key already exists."
    elif [[ -n "${GITHUB_SSH_KEY_SOURCE:-}" ]]; then
        if [[ ! -f "$GITHUB_SSH_KEY_SOURCE" ]]; then
            echo "SSH key source not found: $GITHUB_SSH_KEY_SOURCE"
            return 1
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

        echo "GitHub SSH key copied from source."
    else
        ssh-keygen -t ed25519 -C "$GIT_USER_EMAIL" -f "$key_path" -N ""
        echo "GitHub SSH key generated."
    fi

    configure_github_ssh_config "$key_path"

    echo
    echo "GitHub SSH public key:"
    echo
    cat "$key_path.pub"
    echo
}

configure_github_ssh_config(){
    local key_path="$1"

    touch "$DEV_SSH_CONFIG"

    if ! grep -q "IdentityFile ~/.ssh/$GITHUB_SSH_KEY_NAME" "$DEV_SSH_CONFIG"; then
        cat >> "$DEV_SSH_CONFIG" <<EOF

Host github.com
    HostName github.com
    User git
    IdentityFile ~/.ssh/$GITHUB_SSH_KEY_NAME
    IdentitiesOnly yes
EOF
    fi

    chmod 600 "$DEV_SSH_CONFIG"
    chmod 600 "$key_path"

    if [[ -f "$key_path.pub" ]]; then
        chmod 644 "$key_path.pub"
    fi
}

########################################
# Remove
########################################

remove_dev(){
    log "Removing development environment..."

    restore_git
    remove_github_ssh

    echo "Development cleanup complete."
}

restore_git(){
    if [[ ! -f "$DEV_GIT_BACKUP" ]]; then
        echo "No Git backup found. Skipping Git restore."
        return
    fi

    echo "Restoring Git configuration..."

    git config --global --unset user.name || true
    git config --global --unset user.email || true

    while IFS='=' read -r key value; do
        [[ -n "$key" ]] && git config --global "$key" "$value"
    done < "$DEV_GIT_BACKUP"

    rm -f "$DEV_GIT_BACKUP"
}

remove_github_ssh(){
    if [[ -z "${GITHUB_SSH_KEY_NAME:-}" ]]; then
        echo "GITHUB_SSH_KEY_NAME not set. Skipping GitHub SSH cleanup."
        return
    fi

    local key_path="$DEV_SSH_DIR/$GITHUB_SSH_KEY_NAME"
    local source_path="${GITHUB_SSH_KEY_SOURCE:-}"

    if [[ ! -f "$key_path" ]]; then
        echo "No GitHub SSH key found to remove."
        return
    fi

    if [[ -n "$source_path" && -f "$source_path" ]]; then
        echo "SSH key source already exists: $source_path"
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

            echo "GitHub SSH key saved to source location."
        fi
    fi

    rm -f "$key_path" "$key_path.pub"

    echo "GitHub SSH key removed."
}
