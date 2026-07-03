########################################
# Nginx Configuration
########################################

NGINX_PACKAGES=(
    nginx
)

########################################
# Install
########################################

install_nginx() {
    install_packages "Nginx" "${NGINX_PACKAGES[@]}"

    run systemctl enable nginx
    run systemctl restart nginx
}

########################################
# Remove
########################################

remove_nginx() {
    run systemctl stop nginx || true
    run systemctl disable nginx || true

    remove_packages "Nginx" "${NGINX_PACKAGES[@]}"

    run rm -rf /etc/nginx
    run rm -rf /var/www/html
}

