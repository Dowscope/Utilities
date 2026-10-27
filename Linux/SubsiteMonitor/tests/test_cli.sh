#!/bin/sh
set -eu

program=${1:-./subsite-monitor}
tmp_dir=$(mktemp -d)
trap 'rm -rf "$tmp_dir"' EXIT HUP INT TERM

"$program" --help >"$tmp_dir/help.txt"
grep -q '^Usage: subsite-monitor' "$tmp_dir/help.txt"
grep -q -- '--version' "$tmp_dir/help.txt"

[ "$("$program" --version)" = 'subsite-monitor 0.1.0' ]

if "$program" --unknown >"$tmp_dir/out.txt" 2>"$tmp_dir/error.txt"; then
    echo 'expected an unknown option to fail' >&2
    exit 1
else
    status=$?
fi
[ "$status" -eq 2 ]
grep -q 'unrecognized or incomplete argument' "$tmp_dir/error.txt"

echo 'CLI tests passed'
