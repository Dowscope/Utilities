#!/bin/sh
set -eu

program=${1:-./subsite-monitor}
tmp_dir=$(mktemp -d)
service_pid=

cleanup() {
    if [ -n "$service_pid" ]; then
        kill -TERM "$service_pid" 2>/dev/null || true
        wait "$service_pid" 2>/dev/null || true
    fi
    rm -rf "$tmp_dir"
}
trap cleanup EXIT HUP INT TERM

log_path="$tmp_dir/access.log"
stats_dir="$tmp_dir/stats"
event_csv="$stats_dir/requests-2026-10-27.csv"

printf '%s\n' \
    '2026-10-27T10:00:00+00:00 baseline.test 192.0.2.1 "GET /old HTTP/1.1" 200' \
    >"$log_path"

start_service() {
    SUBSITE_MONITOR_LOG="$log_path" \
    SUBSITE_MONITOR_STATS_DIR="$stats_dir" \
        "$program" service >"$tmp_dir/service.log" 2>&1 &
    service_pid=$!
    attempt=0
    while ! grep -q 'subsite-monitor service: started\|subsite-monitor service: resumed' \
        "$tmp_dir/service.log" 2>/dev/null; do
        attempt=$((attempt + 1))
        if [ "$attempt" -gt 50 ]; then
            cat "$tmp_dir/service.log" >&2
            echo 'monitor service did not start' >&2
            exit 1
        fi
        sleep 0.1
    done
}

wait_for_count() {
    wanted=$1
    attempt=0
    while [ "$(awk 'END { if (NR > 0) print NR - 1; else print 0 }' "$event_csv" 2>/dev/null || echo 0)" -lt "$wanted" ]; do
        attempt=$((attempt + 1))
        if [ "$attempt" -gt 60 ]; then
            cat "$tmp_dir/service.log" >&2
            echo "timed out waiting for $wanted stored requests" >&2
            exit 1
        fi
        sleep 0.1
    done
}

start_service

printf '%s' '2026-10-27T10:00:01+00:00 alpha.test 192.0.2.10 "GET /partial?x=1 HTTP/1.1" 200' \
    >>"$log_path"
sleep 1.2
[ ! -e "$event_csv" ] || [ "$(awk 'END { print NR - 1 }' "$event_csv")" -eq 0 ]
printf '\n' >>"$log_path"
wait_for_count 1

printf '%s\n' \
    '2026-10-27T10:00:02+00:00 alpha.test 192.0.2.11 "POST /submit HTTP/1.1" 201' \
    >>"$log_path"
wait_for_count 2

if SUBSITE_MONITOR_LOG="$log_path" SUBSITE_MONITOR_STATS_DIR="$stats_dir" \
    "$program" collect --log "$log_path" >"$tmp_dir/collect.out" 2>"$tmp_dir/collect.err"; then
    echo 'expected manual collection to be rejected while service is running' >&2
    exit 1
fi
grep -q 'monitor service is running' "$tmp_dir/collect.err"

mv "$log_path" "$log_path.1"
: >"$log_path"
printf '%s\n' \
    '2026-10-27T10:00:03+00:00 beta.test 192.0.2.12 "GET /rotated HTTP/1.1" 200' \
    >>"$log_path"
wait_for_count 3

kill -TERM "$service_pid"
wait "$service_pid"
service_pid=

# Simulate an append that reached the CSV but not its checkpoint.
printf '%s\n' 'uncommitted row' >>"$event_csv"
start_service
wait_for_count 3
! grep -q 'uncommitted row' "$event_csv"

printf '%s\n' \
    '2026-10-27T10:00:04+00:00 beta.test 192.0.2.13 "GET /after-restart HTTP/1.1" 200' \
    >>"$log_path"
wait_for_count 4

: >"$log_path"
attempt=0
while ! grep -q 'access log truncated' "$tmp_dir/service.log"; do
    attempt=$((attempt + 1))
    if [ "$attempt" -gt 30 ]; then
        echo 'monitor service did not detect log truncation' >&2
        exit 1
    fi
    sleep 0.1
done
printf '%s\n' \
    '2026-10-27T10:00:05+00:00 beta.test 192.0.2.14 "GET /after-truncate HTTP/1.1" 200' \
    >>"$log_path"
wait_for_count 5

stats_output=$(SUBSITE_MONITOR_STATS_DIR="$stats_dir" "$program" stats)
printf '%s\n' "$stats_output" | grep -q 'alpha.test.*2'
printf '%s\n' "$stats_output" | grep -q 'beta.test.*3'

echo 'Service tests passed'
