#!/bin/sh
set -eu

script_dir=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
cd "$script_dir"

make -j

if [ "$#" -eq 0 ]; then
    set -- ui --log tests/fixtures/access.log
fi

exec ./subsite-monitor "$@"
