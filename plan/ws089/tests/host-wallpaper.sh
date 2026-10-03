#!/bin/sh
# ws089-p020 (BUG-152): the Wallpaper page's small copies are read by a thread of their own.
#
# Builds the host Settings (host-build.sh) and then look.c again with KEILAND_DATADIR pointing at a folder of the
# test's own under build/ws089-host/wallpaper/data: the default picture and three in the folder (the two of
# userland/desktop/keiland/wallpapers and one that is not a PPM).  Draws the Wallpaper page and checks the log:
#   - the page lists the four pictures and starts the loader before any small copy is read
#     (LOOK pictures count=4, then LOOK loader started count=4, and only then LOOK picture path=...);
#   - three copies are read (error=0), the broken file reports EINVAL (error=22), and LOOK pictures ready count=4.
# The frame is written to build/ws089-host/wallpaper/page.ppm (and page.png when ImageMagick is there) to look at.
#
#   plan/ws089/tests/host-wallpaper.sh
# Copyright (C) 2026 Awe Morris; SPDX-License-Identifier: Zlib
set -eu
cd "$(dirname -- "$0")/../../.."
sh plan/ws089/tests/host-build.sh >/dev/null
host=build/ws089-host
out=$host/wallpaper
data=$(pwd)/$out/data
rm -rf "$out"
mkdir -p "$out/obj" "$data/keiland/wallpapers"
ln -s "$(pwd)/userland/desktop/keiland/wallpapers/Lakeside.ppm" "$data/keiland/wallpaper.ppm"
ln -s "$(pwd)/userland/desktop/keiland/wallpapers/Lakeside.ppm" "$data/keiland/wallpapers/Lakeside.ppm"
ln -s "$(pwd)/userland/desktop/keiland/wallpapers/Birch-Lake.ppm" "$data/keiland/wallpapers/Birch-Lake.ppm"
printf 'not a picture\n' > "$data/keiland/wallpapers/Broken.ppm"

cc=${CC:-cc}
flags="-O2 -g -std=gnu89 -Wall -Wextra -Werror -Wno-unused-parameter -D_GNU_SOURCE -I$host/include -Iuserland/desktop/settings -I."
"$cc" $flags "-DKEILAND_DATADIR=\"$data\"" -c userland/desktop/settings/look.c -o "$out/obj/look.o"
objects=$(ls "$host"/obj/truetype-*.o "$host"/obj/shared-*.o "$host"/obj/settings-*.o | grep -v '/settings-look.o$')
"$cc" -o "$out/settings-render" "$host/obj/host-render.o" "$host/obj/host-network.o" "$out/obj/look.o" $objects -lm -pthread

status=0
timeout 60 "$out/settings-render" --page=wallpaper "draw=$out/page.ppm" > "$out/stdout.log" 2> "$out/log" || status=1
grep 'LOOK' "$out/log" || true
fail() { echo "host-wallpaper: FAIL: $1"; exit 1; }
[ $status = 0 ] || fail "settings-render exited with an error"
grep -q 'ZSETTINGS LOOK pictures count=4$' "$out/log" || fail "the page did not list four pictures"
started=$(grep -n 'ZSETTINGS LOOK loader started count=4$' "$out/log" | head -1 | cut -d: -f1)
first=$(grep -n 'ZSETTINGS LOOK picture path=' "$out/log" | head -1 | cut -d: -f1)
[ -n "$started" ] || fail "no loader"
[ -n "$first" ] || fail "no small copy"
[ "$started" -lt "$first" ] || fail "a small copy was read before the loader started"
[ "$(grep -c 'ZSETTINGS LOOK picture path=.* error=0 ' "$out/log")" = 3 ] || fail "three copies were not read"
grep -q 'ZSETTINGS LOOK picture path=.*/Broken.ppm error=22 ' "$out/log" || fail "the broken file did not report EINVAL"
grep -q 'ZSETTINGS LOOK pictures ready count=4$' "$out/log" || fail "the loader did not finish"
if command -v convert >/dev/null 2>&1; then
	convert "$out/page.ppm" "$out/page.png" && echo "picture: $out/page.png"
fi
echo "host-wallpaper: PASS"
