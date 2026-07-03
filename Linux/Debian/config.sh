########################################
# Runtime Defaults
########################################
DEBIAN_CODENAME="$(. /etc/os-release && echo "$VERSION_CODENAME")"
DEBIAN_VERSION_ID="$(. /etc/os-release && echo "$VERSION_ID")"

########################################
# Modules
########################################

MODULES=(
  packages
  node
  neovim
  treesitter
)

OPTIONAL_MODULES=(
  freeswitch
  nginx
  dotnet
  dev
)

########################################
# Packages
########################################

CORE_PACKAGES=(
  git
  curl
  gnupg2
  lsb-release
  ca-certificates
  apt-transport-https
)

USER_PACKAGES=(
)
