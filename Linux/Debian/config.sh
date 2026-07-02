########################################
# Runtime Defaults
########################################
DEBIAN_CODENAME="$(. /etc/os-release && echo "$VERSION_CODENAME")"

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
  dotnet-sdk-8.0
)
