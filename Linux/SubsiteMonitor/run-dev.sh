#!/bin/sh
set -eu

script_dir=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
cd "$script_dir"

make -j

if [ "$#" -eq 0 ]; then
    if [ -f "$script_dir/docker-data/logs/access.log" ]; then
        default_log="$script_dir/docker-data/logs/access.log"
        default_nginx_config="$script_dir/docker/nginx/nginx.conf"
    else
        default_log="$script_dir/tests/fixtures/access.log"
        default_nginx_config="$script_dir/tests/fixtures/nginx/nginx.conf"
    fi
    SUBSITE_MONITOR_LOG=${SUBSITE_MONITOR_LOG:-"$default_log"}
    SUBSITE_MONITOR_NGINX_CONFIG=${SUBSITE_MONITOR_NGINX_CONFIG:-"$default_nginx_config"}
    export SUBSITE_MONITOR_LOG SUBSITE_MONITOR_NGINX_CONFIG
    exec ./subsite-monitor
fi

exec ./subsite-monitor "$@"
