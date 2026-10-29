#!/bin/sh
set -eu

script_dir=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
version=0.1.0-alpha.7
architecture=$(uname -m)
if [ "$architecture" != x86_64 ]; then
    printf 'Alpha release build currently supports x86_64 only (found %s).\n' \
        "$architecture" >&2
    exit 1
fi

output_dir="$script_dir/packaging/dist"
source_dir="$output_dir/ncurses-source"
ncurses_archive="$source_dir/ncurses-6.5.tar.gz"
ncurses_prefix="$source_dir/ncurses-6.5/static-install"
ncurses_config_stamp="$ncurses_prefix/.subsite-monitor-configured"
stage_dir="$output_dir/stage"
asset="subsite-monitor-$version-linux-x86_64-gnu.tar.gz"
mkdir -p "$source_dir" "$stage_dir/bin" "$stage_dir/Docs" \
    "$stage_dir/config" "$stage_dir/systemd"

if [ ! -f "$ncurses_archive" ]; then
    curl --fail --location --silent --show-error \
        --output "$ncurses_archive" \
        https://invisible-island.net/archives/ncurses/ncurses-6.5.tar.gz
fi
printf '%s  %s\n' \
    136d91bc269a9a5785e5f9e980bc76ab57428f604ce3e5a5a90cebc767971cc6 \
    "$ncurses_archive" | sha256sum --check --status - || {
    printf '%s\n' 'ncurses 6.5 source checksum verification failed.' >&2
    exit 1
}

if [ ! -f "$ncurses_prefix/lib/libncursesw.a" ] ||
    [ ! -f "$ncurses_config_stamp" ]; then
    rm -rf "$source_dir/ncurses-6.5"
    tar -xzf "$ncurses_archive" -C "$source_dir"
    (
        cd "$source_dir/ncurses-6.5"
        ./configure --prefix="$ncurses_prefix" --with-normal \
            --without-shared --without-debug --with-termlib --without-ada \
            --without-cxx --without-tests --without-gpm --without-dlsym \
            --with-terminfo-dirs=/etc/terminfo:/lib/terminfo:/usr/share/terminfo \
            --with-default-terminfo-dir=/usr/share/terminfo
        make -j"$(getconf _NPROCESSORS_ONLN)"
        make install.libs
    )
    touch "$ncurses_config_stamp"
fi

static_cflags="-I$ncurses_prefix/include -I$ncurses_prefix/include/ncursesw"
static_libs="-Wl,--start-group $ncurses_prefix/lib/libncursesw.a $ncurses_prefix/lib/libtinfow.a -Wl,--end-group"

make -C "$script_dir" clean
make -C "$script_dir" -j VERSION="$version" \
    LDFLAGS='-static' NCURSES_CFLAGS="$static_cflags" \
    NCURSES_LIBS="$static_libs" test

make -C "$script_dir" clean
make -C "$script_dir" -j VERSION="$version" \
    LDFLAGS='-static' NCURSES_CFLAGS="$static_cflags" \
    NCURSES_LIBS="$static_libs"

install -m 0755 "$script_dir/subsite-monitor" "$stage_dir/bin/subsite-monitor"
strip --strip-unneeded "$stage_dir/bin/subsite-monitor"
install -m 0644 "$script_dir/README.md" "$stage_dir/README.md"
install -m 0644 "$script_dir/Docs/MANUAL.md" "$stage_dir/Docs/MANUAL.md"
install -m 0644 "$script_dir/config/monitor.conf.example" \
    "$stage_dir/config/monitor.conf.example"
install -m 0644 "$script_dir/config/nginx-log-format.conf.example" \
    "$stage_dir/config/nginx-log-format.conf.example"
install -m 0644 "$script_dir/config/subsite-monitor.logrotate" \
    "$stage_dir/config/subsite-monitor.logrotate"
install -m 0644 "$script_dir/systemd/subsite-monitor.service" \
    "$stage_dir/systemd/subsite-monitor.service"

if ldd "$stage_dir/bin/subsite-monitor" 2>&1 | grep -q 'not a dynamic executable'; then
    :
else
    printf '%s\n' 'Release binary has dynamic library dependencies.' >&2
    exit 1
fi

tar --sort=name --mtime='UTC 1970-01-01' --owner=0 --group=0 --numeric-owner \
    -cf - -C "$stage_dir" bin README.md Docs/MANUAL.md \
    config/monitor.conf.example config/nginx-log-format.conf.example \
    config/subsite-monitor.logrotate \
    systemd/subsite-monitor.service |
    gzip -n >"$output_dir/$asset"
(cd "$output_dir" && sha256sum "$asset" >SHA256SUMS)

asset_hash=$(sha256sum "$output_dir/$asset" | awk '{print $1}')
for installer in installer.sh install.sh; do
    sed -e "s/@VERSION@/$version/g" \
        -e "s/@ASSET_SHA256@/$asset_hash/g" \
        "$script_dir/packaging/installer.sh.in" >"$script_dir/$installer"
    chmod 0755 "$script_dir/$installer"
    sh -n "$script_dir/$installer"
done

sh "$script_dir/tests/test_installer.sh" "$output_dir/$asset"
make -C "$script_dir" clean
rm -rf "$stage_dir"

printf 'Alpha asset ready: %s\n' "$output_dir/$asset"
printf 'SHA256: %s\n' "$asset_hash"
