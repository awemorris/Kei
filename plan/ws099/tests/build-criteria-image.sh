#!/bin/sh
# ws099-p001: builds the criteria image (config-amd64-criteria.mk) like plan/ws035/tests/build-login-image.sh's
# graphical one, with the generated wallpapers in /usr/share/keiland/wallpapers (userland/desktop/wallpapers/generate.py,
# as the demonstration image and plan/ws089/tests/build-settings-image.sh have them).
#
#   plan/ws099/tests/build-criteria-image.sh [BUILD]     (default build/amd64)
# Copyright (C) 2026 Awe Morris; SPDX-License-Identifier: Zlib
set -eu
cd "$(dirname -- "$0")/../../.."
. plan/tools/guest/jobs.sh  # ZEDBSD_JOBS, the parallel jobs (default 16)
build=${1:-build/amd64}
extra=$(python3 plan/tools/guest/guest.py extra-files | sed -n "s/^ZEDBSD_TEST_EXTRA_FILES='\(.*\)'$/\1/p")
[ -n "$extra" ] || { echo "build-criteria-image: no guest files (plan/tools/guest/guest.py keys?)"; exit 1; }
[ -f build/ws035-fonts/Inter.ttf ] && extra="$extra --file /usr/share/fonts/keiland.ttf=build/ws035-fonts/Inter.ttf"
[ -f build/ws035-fonts/OFL.txt ] && extra="$extra --file /usr/share/fonts/keiland-OFL.txt=build/ws035-fonts/OFL.txt"
[ -f build/ws035-fonts/JetBrainsMono-Regular.ttf ] && extra="$extra --file /usr/share/fonts/keiland-mono.ttf=build/ws035-fonts/JetBrainsMono-Regular.ttf"
[ -f build/ws035-fonts/JetBrainsMono-OFL.txt ] && extra="$extra --file /usr/share/fonts/keiland-mono-OFL.txt=build/ws035-fonts/JetBrainsMono-OFL.txt"
[ -f build/ws035-wallpaper/wallpaper.ppm ] && extra="$extra --file /usr/share/keiland/wallpaper.ppm=build/ws035-wallpaper/wallpaper.ppm"
extra="$extra --file /usr/share/files-tests/make-home.sh=plan/tools/files/make-home.sh"
python3 userland/desktop/wallpapers/generate.py "$build/wallpapers" >/dev/null
for picture in "$build"/wallpapers/*.ppm; do
	extra="$extra --file /usr/share/keiland/wallpapers/$(basename "$picture")=$picture"
done
exec make -j"$ZEDBSD_JOBS" ZEDBSD_CONFIG=plan/ws099/tests/config-amd64-criteria.mk BUILD="$build" \
    "ZEDBSD_TEST_EXTRA_FILES=$extra" disk-image
