#!/bin/sh
set -eu

program=${1:-./subsite-monitor}
tmp_dir=$(mktemp -d)
trap 'rm -rf "$tmp_dir"' EXIT HUP INT TERM

"$program" --help >"$tmp_dir/help.txt"
grep -q '^Usage:' "$tmp_dir/help.txt"
grep -q -- '--version' "$tmp_dir/help.txt"
grep -q 'collect --log <path>' "$tmp_dir/help.txt"
grep -q 'stats --log <path>' "$tmp_dir/help.txt"
grep -q 'ui --log <path>' "$tmp_dir/help.txt"

[ "$("$program" --version)" = 'subsite-monitor 0.1.0-alpha.1' ]

if "$program" --unknown >"$tmp_dir/out.txt" 2>"$tmp_dir/error.txt"; then
    echo 'expected an unknown option to fail' >&2
    exit 1
else
    status=$?
fi
[ "$status" -eq 2 ]
grep -q 'unrecognized or incomplete argument' "$tmp_dir/error.txt"

"$program" collect --log tests/fixtures/access.log >"$tmp_dir/collect.txt"
grep -q '^Parsed records: 5$' "$tmp_dir/collect.txt"
grep -q '^Malformed lines: 2$' "$tmp_dir/collect.txt"
if grep -Eq '203\.0\.113|/private\?' "$tmp_dir/collect.txt"; then
    echo 'collect output must not expose client IPs or request targets' >&2
    exit 1
fi

"$program" stats --log tests/fixtures/access.log >"$tmp_dir/stats.txt"
grep -q 'Website.*Requests.*Last access (UTC)' "$tmp_dir/stats.txt"
grep -q '2026-10-27T14:45:32Z' "$tmp_dir/stats.txt"
grep -q 'Daily requests by website' "$tmp_dir/stats.txt"
grep -q '^Malformed lines: 2$' "$tmp_dir/stats.txt"
if grep -Eq '203\.0\.113|/private\?' "$tmp_dir/stats.txt"; then
    echo 'stats output must not expose client IPs or request targets' >&2
    exit 1
fi

if "$program" stats >"$tmp_dir/out.txt" 2>"$tmp_dir/error.txt"; then
    echo 'expected stats without a log path to fail at this stage' >&2
    exit 1
else
    status=$?
fi
[ "$status" -eq 2 ]
grep -q 'stats currently requires --log <path>' "$tmp_dir/error.txt"

if "$program" ui --log tests/fixtures/access.log \
    >"$tmp_dir/out.txt" 2>"$tmp_dir/error.txt"; then
    echo 'expected UI to reject non-interactive output' >&2
    exit 1
else
    status=$?
fi
[ "$status" -eq 1 ]
grep -q 'ui requires an interactive terminal' "$tmp_dir/error.txt"

awk 'BEGIN {
    for (i = 0; i < 10001; ++i)
        printf "2026-10-27T14:45:32+00:00 host%d.example.test 192.0.2.1 \"GET / HTTP/1.1\" 200\n", i
}' >"$tmp_dir/many-sites.log"
if "$program" stats --log "$tmp_dir/many-sites.log" \
    >"$tmp_dir/out.txt" 2>"$tmp_dir/error.txt"; then
    echo 'expected excessive unique sites to fail safely' >&2
    exit 1
else
    status=$?
fi
[ "$status" -eq 1 ]
grep -q 'too many distinct websites' "$tmp_dir/error.txt"

awk 'BEGIN { for (i = 0; i < 17000; ++i) printf "x"; printf "\n" }' \
    >"$tmp_dir/oversized.log"
"$program" collect --log "$tmp_dir/oversized.log" >"$tmp_dir/oversized.txt"
grep -q '^Parsed records: 0$' "$tmp_dir/oversized.txt"
grep -q '^Malformed lines: 1$' "$tmp_dir/oversized.txt"

printf '%s' '2026-10-27T14:45:32+00:00 example.test 192.0.2.1 "GET / HTTP/1.1" 200' \
    >"$tmp_dir/no-final-newline.log"
"$program" collect --log "$tmp_dir/no-final-newline.log" \
    >"$tmp_dir/no-final-newline.txt"
grep -q '^Parsed records: 1$' "$tmp_dir/no-final-newline.txt"
grep -q '^Malformed lines: 0$' "$tmp_dir/no-final-newline.txt"

if "$program" collect >"$tmp_dir/out.txt" 2>"$tmp_dir/error.txt"; then
    echo 'expected incomplete collect arguments to fail' >&2
    exit 1
else
    status=$?
fi
[ "$status" -eq 2 ]
grep -q 'collect requires --log <path>' "$tmp_dir/error.txt"

if "$program" collect --log "$tmp_dir/missing.log" >"$tmp_dir/out.txt" 2>"$tmp_dir/error.txt"; then
    echo 'expected a missing log path to fail' >&2
    exit 1
else
    status=$?
fi
[ "$status" -eq 1 ]
grep -q 'cannot open access log' "$tmp_dir/error.txt"

echo 'CLI tests passed'
