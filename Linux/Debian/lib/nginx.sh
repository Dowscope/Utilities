########################################
# Nginx Configuration
########################################

NGINX_PACKAGES=(
    nginx
)

NGINX_CONFIG_URL="$REPO_BASE/config/nginx/default"
NGINX_AVAILABLE="/etc/nginx/sites-available/default"
NGINX_ENABLED="/etc/nginx/sites-enabled/default"

########################################
# Configure
########################################

configure_nginx() {
    log "Configuring Nginx"

    curl -fsSL "$NGINX_CONFIG_URL" -o "$RUN_TMP/default"

    run cp "$RUN_TMP/default" "$NGINX_AVAILABLE"
    run ln -sfn "$NGINX_AVAILABLE" "$NGINX_ENABLED"

    run nginx -t
}

########################################
# Install
########################################

install_nginx() {
    install_packages "Nginx" "${NGINX_PACKAGES[@]}"

    configure_nginx

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
