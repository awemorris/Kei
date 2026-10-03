#!/bin/sh
# ws079-p005: builds the lean zdesktop guest image with Notes (plan/ws079/tests/config-amd64-notes.mk)
# with the guest harness's files, the fonts, the wallpaper and the tests' sample home maker,
# like plan/tools/titlebar/build-menu-image.sh.  The fonts and the wallpaper are not in git:
# build/ws035-fonts (Inter, JetBrains Mono), build/ws035-wallpaper, and build/ws071-fonts
# (DroidSansFallbackFull.ttf, Apache-2.0, for Japanese file names; copied from the host's
# /usr/share/fonts/truetype/droid/ with /usr/share/doc/fonts-droid-fallback/copyright).
#
#   plan/ws079/tests/build-notes-image.sh [BUILD]     (default build/amd64)
# Copyright (C) 2026 Awe Morris; SPDX-License-Identifier: Zlib
set -eu
cd "$(dirname -- "$0")/../../.."
. plan/tools/guest/jobs.sh  # ZEDBSD_JOBS, the parallel jobs (default 16)
build=${1:-build/amd64}
extra=$(python3 plan/tools/guest/guest.py extra-files | sed -n "s/^ZEDBSD_TEST_EXTRA_FILES='\(.*\)'$/\1/p")
[ -n "$extra" ] || { echo "build-notes-image: no guest files (plan/tools/guest/guest.py keys?)"; exit 1; }
[ -f build/ws035-fonts/Inter.ttf ] && extra="$extra --file /usr/share/fonts/keiland.ttf=build/ws035-fonts/Inter.ttf"
[ -f build/ws035-fonts/OFL.txt ] && extra="$extra --file /usr/share/fonts/keiland-OFL.txt=build/ws035-fonts/OFL.txt"
[ -f build/ws035-fonts/JetBrainsMono-Regular.ttf ] && extra="$extra --file /usr/share/fonts/keiland-mono.ttf=build/ws035-fonts/JetBrainsMono-Regular.ttf"
[ -f build/ws035-fonts/JetBrainsMono-OFL.txt ] && extra="$extra --file /usr/share/fonts/keiland-mono-OFL.txt=build/ws035-fonts/JetBrainsMono-OFL.txt"
[ -f build/ws071-fonts/DroidSansFallbackFull.ttf ] && extra="$extra --file /usr/share/fonts/keiland-fallback.ttf=build/ws071-fonts/DroidSansFallbackFull.ttf"
[ -f build/ws071-fonts/DroidSansFallback-LICENSE.txt ] && extra="$extra --file /usr/share/fonts/keiland-fallback-LICENSE.txt=build/ws071-fonts/DroidSansFallback-LICENSE.txt"
[ -f build/ws035-wallpaper/wallpaper.ppm ] && extra="$extra --file /usr/share/keiland/wallpaper.ppm=build/ws035-wallpaper/wallpaper.ppm"
extra="$extra --file /usr/share/files-tests/make-home.sh=plan/tools/files/make-home.sh"
exec make -j"$ZEDBSD_JOBS" ZEDBSD_CONFIG=plan/ws079/tests/config-amd64-notes.mk BUILD="$build" \
    "ZEDBSD_TEST_EXTRA_FILES=$extra" disk-image
