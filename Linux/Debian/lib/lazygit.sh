LAZYGIT_VERSION="latest"
LAZYGIT_INSTALL_PATH="/usr/local/bin/lazygit"
LAZYGIT_REPO="https://github.com/jesseduffield/lazygit"
LAZYGIT_API="https://api.github.com/repos/jesseduffield/lazygit/releases/latest"

install_lazygit() {
  log "Installing LazyGit"

  if command_exists lazygit; then
    log "lazygit already installed"
    return 0
  fi

  run apt-get update
  run apt-get install -y curl tar

  LAZYGIT_ARCH="$(uname -m)"
  case "$LAZYGIT_ARCH" in
    x86_64) LAZYGIT_ARCH="x86_64" ;;
    aarch64|arm64) LAZYGIT_ARCH="arm64" ;;
    *)
      echo "Unsupported architecture: $LAZYGIT_ARCH"
      return 1
      ;;
  esac

  if [ "$LAZYGIT_VERSION" = "latest" ]; then
    LAZYGIT_DOWNLOAD_VERSION="$(
      curl -s "$LAZYGIT_API" \
      | grep -Po '"tag_name": *"v\K[^"]*'
    )"
  else
    LAZYGIT_DOWNLOAD_VERSION="$LAZYGIT_VERSION"
  fi

  LAZYGIT_TMP_FILE="/tmp/lazygit.tar.gz"
  LAZYGIT_TMP_DIR="/tmp/lazygit-install"

  rm -rf "$LAZYGIT_TMP_DIR"
  mkdir -p "$LAZYGIT_TMP_DIR"

  run curl -L \
    -o "$LAZYGIT_TMP_FILE" \
    "$LAZYGIT_REPO/releases/download/v${LAZYGIT_DOWNLOAD_VERSION}/lazygit_${LAZYGIT_DOWNLOAD_VERSION}_Linux_${LAZYGIT_ARCH}.tar.gz"

  run tar xf "$LAZYGIT_TMP_FILE" -C "$LAZYGIT_TMP_DIR" lazygit

  run install "$LAZYGIT_TMP_DIR/lazygit" -D -m 755 "$LAZYGIT_INSTALL_PATH"

  rm -rf "$LAZYGIT_TMP_FILE" "$LAZYGIT_TMP_DIR"

  echo "LazyGit installed"
}

remove_lazygit() {
  log "Removing LazyGit"

  run rm -f "$LAZYGIT_INSTALL_PATH"

  echo "LazyGit removed"
}
