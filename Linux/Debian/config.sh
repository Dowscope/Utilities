########################################
# Sudo mode (overridden by --root)
########################################

USE_SUDO=true

########################################
# Packages
########################################

CORE_PACKAGES=(
    git
    curl
    unzip
)

USER_PACKAGES=(
    ripgrep
    fd-find
    fzf
)
