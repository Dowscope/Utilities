#!/bin/sh
set -eu

script_dir=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
cd "$script_dir"

if ! command -v docker >/dev/null 2>&1; then
    printf '%s\n' 'run-docker.sh: Docker is not installed.' >&2
    exit 1
fi

if ! docker compose version >/dev/null 2>&1; then
    printf '%s\n' 'run-docker.sh: Docker Compose v2 is required.' >&2
    exit 1
fi

mkdir -p docker-data/logs

if [ "$#" -eq 0 ]; then
    set -- up --remove-orphans
fi

exec docker compose "$@"
