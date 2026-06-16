########################################
# Runtime Defaults
########################################

USE_SUDO=true
MODE="install"

INSTALL_FREESWITCH=false

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

FREESWITCH_PACKAGES=(
  freeswitch
  freeswitch-mod-sofia
  freeswitch-mod-conference
  freeswitch-mod-event-socket
  freeswitch-mod-commands
  freeswitch-mod-db
  freeswitch-mod-console
  freeswitch-sounds-en-us-callie
  freeswitch-music-default
)
