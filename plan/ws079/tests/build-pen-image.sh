#!/bin/sh
# ws079: builds the pen test image (plan/ws079/tests/config-amd64-pen.mk) with
# the guest harness's files, the fonts and the wallpaper, like
# plan/tools/titlebar/build-menu-image.sh.
#
#   plan/ws079/tests/build-pen-image.sh [BUILD]     (default build/amd64)
# Copyright (C) 2026 Awe Morris; SPDX-License-Identifier: Zlib
set -eu
cd "$(dirname -- "$0")/../../.."
. plan/tools/guest/jobs.sh  # ZEDBSD_JOBS, the parallel jobs (default 16)
build=${1:-build/amd64}
extra=$(python3 plan/tools/guest/guest.py extra-files | sed -n "s/^ZEDBSD_TEST_EXTRA_FILES='\(.*\)'$/\1/p")
[ -n "$extra" ] || { echo "build-pen-image: no guest files (plan/tools/guest/guest.py keys?)"; exit 1; }
[ -f build/ws035-fonts/Inter.ttf ] && extra="$extra --file /usr/share/fonts/keiland.ttf=build/ws035-fonts/Inter.ttf"
[ -f build/ws035-fonts/OFL.txt ] && extra="$extra --file /usr/share/fonts/keiland-OFL.txt=build/ws035-fonts/OFL.txt"
[ -f build/ws035-fonts/JetBrainsMono-Regular.ttf ] && extra="$extra --file /usr/share/fonts/keiland-mono.ttf=build/ws035-fonts/JetBrainsMono-Regular.ttf"
[ -f build/ws035-fonts/JetBrainsMono-OFL.txt ] && extra="$extra --file /usr/share/fonts/keiland-mono-OFL.txt=build/ws035-fonts/JetBrainsMono-OFL.txt"
[ -f build/ws035-wallpaper/wallpaper.ppm ] && extra="$extra --file /usr/share/keiland/wallpaper.ppm=build/ws035-wallpaper/wallpaper.ppm"
exec make -j"$ZEDBSD_JOBS" ZEDBSD_CONFIG=plan/ws079/tests/config-amd64-pen.mk BUILD="$build" \
    "ZEDBSD_TEST_EXTRA_FILES=$extra" disk-image
