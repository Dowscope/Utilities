#!/bin/sh
set -eu
TZ=UTC0
export TZ

program=${1:-./subsite-monitor}
tmp_dir=$(mktemp -d)
trap 'rm -rf "$tmp_dir"' EXIT HUP INT TERM

"$program" --help >"$tmp_dir/help.txt"
grep -q '^Usage:' "$tmp_dir/help.txt"
grep -q -- '--version' "$tmp_dir/help.txt"
grep -q 'With no arguments, open the UI' "$tmp_dir/help.txt"
grep -q 'collect --log <path>' "$tmp_dir/help.txt"
grep -q 'stats --log <path>' "$tmp_dir/help.txt"
grep -q 'stats \[--days <count>\]' "$tmp_dir/help.txt"
grep -q 'ui --log <path>' "$tmp_dir/help.txt"
grep -q 'https://github.com/Dowscope/Utilities/issues' "$tmp_dir/help.txt"

[ "$("$program" --version)" = 'subsite-monitor 0.1.0-alpha.12' ]

if "$program" --unknown >"$tmp_dir/out.txt" 2>"$tmp_dir/error.txt"; then
    echo 'expected an unknown option to fail' >&2
    exit 1
else
    status=$?
fi
[ "$status" -eq 2 ]
grep -q 'unrecognized or incomplete argument' "$tmp_dir/error.txt"

if SUBSITE_MONITOR_LOG=tests/fixtures/access.log "$program" \
    </dev/null >"$tmp_dir/out.txt" 2>"$tmp_dir/error.txt"; then
    echo 'expected no-argument UI mode to reject non-interactive output' >&2
    exit 1
else
    status=$?
fi
[ "$status" -eq 1 ]
grep -q 'ui requires an interactive terminal' "$tmp_dir/error.txt"

if SUBSITE_MONITOR_LOG=tests/fixtures/access.log "$program" ui \
    </dev/null >"$tmp_dir/out.txt" 2>"$tmp_dir/error.txt"; then
    echo 'expected default ui mode to reject non-interactive output' >&2
    exit 1
else
    status=$?
fi
[ "$status" -eq 1 ]
grep -q 'ui requires an interactive terminal' "$tmp_dir/error.txt"

if command -v script >/dev/null 2>&1; then
    printf 'q' | TERM=xterm \
        SUBSITE_MONITOR_LOG="$tmp_dir/missing-access.log" \
        SUBSITE_MONITOR_NGINX_CONFIG=tests/fixtures/nginx/nginx.conf \
        script -q -e -c "$program" /dev/null >"$tmp_dir/ui-missing-log.txt"
    grep -q 'alpha.example.test' "$tmp_dir/ui-missing-log.txt"
    grep -q 'v0.1.0-alpha.12' "$tmp_dir/ui-missing-log.txt"
    grep -q 'https://github.com/Dowscope/Utilities/issues' \
        "$tmp_dir/ui-missing-log.txt"
    grep -q 'Access log unavailable; showing configured sites only.' \
        "$tmp_dir/ui-missing-log.txt"
    grep -q 'Sort: 1-3.*Select: Up/Dn.*Details: Enter.*Refresh: r.*Quit: q' \
        "$tmp_dir/ui-missing-log.txt"
    grep -q 'LAST ACCESS' "$tmp_dir/ui-missing-log.txt"
    if grep -q 'u Update' "$tmp_dir/ui-missing-log.txt"; then
        echo 'the command bar must not show an update shortcut' >&2
        exit 1
    fi

    mkdir -p "$tmp_dir/mock-bin"
    cat >"$tmp_dir/mock-bin/python3" <<'MOCK_PYTHON'
#!/bin/sh
printf '0.1.0-alpha.13\t2026-10-30\tU3Vic2l0ZSBNb25pdG9yIGFscGhhLjEz\tUmVwYWlyZWQgYnVncyBhbmQgaW1wcm92ZWQgdXBkYXRlciBzYWZldHku\n'
MOCK_PYTHON
    chmod 0755 "$tmp_dir/mock-bin/python3"
    { sleep 1; printf 'q'; } | TERM=xterm PATH="$tmp_dir/mock-bin:$PATH" \
        SUBSITE_MONITOR_LOG=tests/fixtures/access.log \
        SUBSITE_MONITOR_NGINX_CONFIG=tests/fixtures/nginx/nginx.conf \
        SUBSITE_MONITOR_STATS_DIR="$tmp_dir/ui-badge-stats" \
        script -q -e -c "$program" /dev/null >"$tmp_dir/ui-badge.txt"
    grep -q 'Update 0.1.0-alpha.13 (u)' "$tmp_dir/ui-badge.txt"

    printf 'u\nnqq' | TERM=xterm PATH="$tmp_dir/mock-bin:$PATH" \
        SUBSITE_MONITOR_LOG=tests/fixtures/access.log \
        SUBSITE_MONITOR_NGINX_CONFIG=tests/fixtures/nginx/nginx.conf \
        SUBSITE_MONITOR_STATS_DIR="$tmp_dir/ui-updater-stats" \
        script -q -e -c "$program" /dev/null >"$tmp_dir/ui-updater.txt"
    grep -q 'alpha.13' "$tmp_dir/ui-updater.txt"
    grep -q 'Repair' "$tmp_dir/ui-updater.txt"
    grep -q 'CONFIRM' "$tmp_dir/ui-updater.txt"

    cat >"$tmp_dir/mock-bin/python3" <<'MOCK_NO_UPDATE'
#!/bin/sh
exit 0
MOCK_NO_UPDATE
    chmod 0755 "$tmp_dir/mock-bin/python3"
    { sleep 1; printf 'q'; } | TERM=xterm PATH="$tmp_dir/mock-bin:$PATH" \
        SUBSITE_MONITOR_LOG=tests/fixtures/access.log \
        SUBSITE_MONITOR_NGINX_CONFIG=tests/fixtures/nginx/nginx.conf \
        SUBSITE_MONITOR_STATS_DIR="$tmp_dir/ui-no-update-stats" \
        script -q -e -c "$program" /dev/null >"$tmp_dir/ui-no-update.txt"
    if grep -q 'Up to date\|Check failed\|Checking updates' \
        "$tmp_dir/ui-no-update.txt"; then
        echo 'the update indicator should be hidden when no update is available' >&2
        exit 1
    fi

    printf '\n2q' | TERM=xterm \
        SUBSITE_MONITOR_LOG=tests/fixtures/access.log \
        SUBSITE_MONITOR_NGINX_CONFIG=tests/fixtures/nginx/nginx.conf \
        SUBSITE_MONITOR_STATS_DIR="$tmp_dir/ui-stats" \
        script -q -e -c "$program" /dev/null >"$tmp_dir/ui-history.txt"
    grep -q 'HISTORY' "$tmp_dir/ui-history.txt"
    grep -q 'v0.1.0-alpha.12' "$tmp_dir/ui-history.txt"
    grep -q 'TIME' "$tmp_dir/ui-history.txt"
    grep -Eq 'Sort: ?1-4' "$tmp_dir/ui-history.txt"
    grep -Eq 'Rows: ?Up/Dn' "$tmp_dir/ui-history.txt"
    grep -Eq 'Pages?: ?PgUp/Dn' "$tmp_dir/ui-history.txt"
    grep -Eq 'URL: ?L/R' "$tmp_dir/ui-history.txt"
    grep -Eq 'Back: ?Esc' "$tmp_dir/ui-history.txt"
    newest_site_position=$(grep -abo 'live.lakeheadschools.ca' \
        "$tmp_dir/ui-history.txt" | head -n 1 | cut -d: -f1)
    older_site_position=$(grep -abo 'test.lakeheadschools.ca' \
        "$tmp_dir/ui-history.txt" | head -n 1 | cut -d: -f1)
    [ "$newest_site_position" -lt "$older_site_position" ]
    newest_position=$(grep -abo '/health' "$tmp_dir/ui-history.txt" |
        head -n 1 | cut -d: -f1)
    older_position=$(grep -abo '/check' "$tmp_dir/ui-history.txt" |
        head -n 1 | cut -d: -f1)
    oldest_position=$(grep -abo '/forms/submit' "$tmp_dir/ui-history.txt" |
        head -n 1 | cut -d: -f1)
    [ "$newest_position" -lt "$older_position" ]
    [ "$older_position" -lt "$oldest_position" ]
fi

"$program" collect --log tests/fixtures/access.log \
    --stats-dir "$tmp_dir/stats" >"$tmp_dir/collect.txt"
grep -q '^Parsed records: 5$' "$tmp_dir/collect.txt"
grep -q '^Malformed lines: 2$' "$tmp_dir/collect.txt"
grep -q '/private?token=hidden,404' \
    "$tmp_dir/stats/requests-2026-10-27.csv"
grep -q '203.0.113.11' "$tmp_dir/stats/requests-2026-10-27.csv"
if grep -Eq '203\.0\.113|/private\?' "$tmp_dir/collect.txt"; then
    echo 'collect output must not expose client IPs or request targets' >&2
    exit 1
fi

"$program" collect --log tests/fixtures/access.log \
    --stats-dir "$tmp_dir/stats" >"$tmp_dir/collect-repeat.txt"
"$program" stats --stats-dir "$tmp_dir/stats" >"$tmp_dir/stored-stats.txt"
grep -q 'live.lakeheadschools.ca.*4' "$tmp_dir/stored-stats.txt"
grep -q 'test.lakeheadschools.ca.*1' "$tmp_dir/stored-stats.txt"
"$program" stats --days 1 --stats-dir "$tmp_dir/stats" \
    >"$tmp_dir/recent-stats.txt"
grep -q '2026-10-27' "$tmp_dir/recent-stats.txt"
if grep -q '2026-10-26' "$tmp_dir/recent-stats.txt"; then
    echo 'stats --days 1 should limit output to the newest stored date' >&2
    exit 1
fi

"$program" stats --log tests/fixtures/access.log >"$tmp_dir/stats.txt"
grep -q 'Website.*Requests.*Last access (server time)' "$tmp_dir/stats.txt"
grep -q '2026-10-27 14:45:32 +0000' "$tmp_dir/stats.txt"
grep -q 'Daily requests by website' "$tmp_dir/stats.txt"
grep -q '^Malformed lines: 2$' "$tmp_dir/stats.txt"
if grep -Eq '203\.0\.113|/private\?' "$tmp_dir/stats.txt"; then
    echo 'stats output must not expose client IPs or request targets' >&2
    exit 1
fi

if "$program" stats --days 0 --stats-dir "$tmp_dir/stats" \
    >"$tmp_dir/out.txt" 2>"$tmp_dir/error.txt"; then
    echo 'expected zero days to be rejected' >&2
    exit 1
else
    status=$?
fi
[ "$status" -eq 2 ]
grep -q -- '--days must be an integer' "$tmp_dir/error.txt"

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
"$program" collect --log "$tmp_dir/oversized.log" \
    --stats-dir "$tmp_dir/stats" >"$tmp_dir/oversized.txt"
grep -q '^Parsed records: 0$' "$tmp_dir/oversized.txt"
grep -q '^Malformed lines: 1$' "$tmp_dir/oversized.txt"

printf '%s' '2026-10-27T14:45:32+00:00 example.test 192.0.2.1 "GET / HTTP/1.1" 200' \
    >"$tmp_dir/no-final-newline.log"
"$program" collect --log "$tmp_dir/no-final-newline.log" \
    --stats-dir "$tmp_dir/stats" \
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
