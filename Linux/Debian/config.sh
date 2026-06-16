########################################
# Packages
########################################

MODULES=(
  packages
  node
  neovim
  treesitter
  freeswitch
)

CORE_PACKAGES=(
  git
  curl
)

USER_PACKAGES=(
  gnupg2
  lsb-release
  ca-certificates
  apt-transport-https
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
