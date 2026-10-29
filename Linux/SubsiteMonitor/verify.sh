#!/bin/sh
set -eu

script_dir=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
cd "$script_dir"

make clean
make -j
make test
./subsite-monitor --help >/dev/null
version_output=$(./subsite-monitor --version)
[ "$version_output" = 'subsite-monitor 0.1.0-alpha.10' ]

printf '%s\n' 'Verification passed: build, tests, help, and version.'
