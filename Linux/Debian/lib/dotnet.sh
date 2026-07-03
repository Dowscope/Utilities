########################################
# Dotnet Configuration
########################################

DOTNET_PACKAGES=(
  dotnet-runtime-8.0
)

DOTNET_REPO_DEB="/tmp/packages-microsoft-prod.deb"
DOTNET_REPO_URL="https://packages.microsoft.com/config/debian/$DEBIAN_VERSION_ID/packages-microsoft-prod.deb"

########################################
# Install
########################################

install_dotnet() {
  log "Installing DOTNET..."

  install_dotnet_repo
  install_packages "Dotnet Packages" "${DOTNET_PACKAGES[@]}"
}

install_dotnet_repo() {
  echo "Installing Microsoft package repository..."

  if [ -f /etc/apt/sources.list.d/microsoft-prod.list ] || [ -f /etc/apt/sources.list.d/microsoft-prod.sources ]; then
    echo "Microsoft package repository already exists."
    return 0
  fi

  curl -fsSL "$DOTNET_REPO_URL" -o "$DOTNET_REPO_DEB"
  run dpkg -i "$DOTNET_REPO_DEB"
  rm -f "$DOTNET_REPO_DEB"
  run apt-get update
}


########################################
# Remove
########################################

remove_dotnet() {
  log "Removing DOTNET"
  remove_packages "Dotnet Packages" "${DOTNET_PACKAGES[@]}"
  remove_dotnet_repo
}


remove_dotnet_repo() {
  echo "Removing Microsoft package repository..."

  run rm -f /etc/apt/sources.list.d/microsoft-prod.list
  run rm -f /etc/apt/sources.list.d/microsoft-prod.sources
  run rm -f /etc/apt/trusted.gpg.d/microsoft-prod.gpg
  run rm -f /usr/share/keyrings/microsoft-prod.gpg
  run apt-get update || true
}
