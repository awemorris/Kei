#!/bin/sh
# ws095-p013: builds the lean Venus guest image with the input method and the Text Editor
# (plan/ws095/tests/config-amd64-ime-textedit.mk), with the guest harness's files, the fonts (Inter,
# JetBrains Mono, Droid Sans Fallback for Japanese) and the wallpaper, like plan/tools/files/build-files-image.sh.
# The fonts and the wallpaper are not in git (build/ws035-fonts, build/ws035-wallpaper).
#
#   plan/ws095/tests/build-ime-textedit-image.sh [BUILD]     (default build/ws095/img-textedit)
# Copyright (C) 2026 Awe Morris; SPDX-License-Identifier: Zlib
set -eu
cd "$(dirname -- "$0")/../../.."
build=${1:-build/ws095/img-textedit}
extra=$(python3 plan/tools/guest/guest.py extra-files | sed -n "s/^ZEDBSD_TEST_EXTRA_FILES='\(.*\)'$/\1/p")
[ -n "$extra" ] || { echo "build-ime-textedit-image: no guest files (plan/tools/guest/guest.py keys?)"; exit 1; }
fonts=build/ws035-fonts
[ -f $fonts/Inter.ttf ] && extra="$extra --file /usr/share/fonts/keiland.ttf=$fonts/Inter.ttf"
[ -f $fonts/OFL.txt ] && extra="$extra --file /usr/share/fonts/keiland-OFL.txt=$fonts/OFL.txt"
[ -f $fonts/JetBrainsMono-Regular.ttf ] && extra="$extra --file /usr/share/fonts/keiland-mono.ttf=$fonts/JetBrainsMono-Regular.ttf"
[ -f $fonts/JetBrainsMono-OFL.txt ] && extra="$extra --file /usr/share/fonts/keiland-mono-OFL.txt=$fonts/JetBrainsMono-OFL.txt"
[ -f $fonts/DroidSansFallbackFull.ttf ] && extra="$extra --file /usr/share/fonts/keiland-fallback.ttf=$fonts/DroidSansFallbackFull.ttf"
[ -f $fonts/DroidSansFallback-LICENSE.txt ] && extra="$extra --file /usr/share/fonts/keiland-fallback-LICENSE.txt=$fonts/DroidSansFallback-LICENSE.txt"
[ -f build/ws035-wallpaper/wallpaper.ppm ] && extra="$extra --file /usr/share/keiland/wallpaper.ppm=build/ws035-wallpaper/wallpaper.ppm"
extra="$extra --file /usr/share/files-tests/make-home.sh=plan/tools/files/make-home.sh"
exec make -j"$(nproc)" ZEDBSD_CONFIG=plan/ws095/tests/config-amd64-ime-textedit.mk BUILD="$build" \
    "ZEDBSD_TEST_EXTRA_FILES=$extra" disk-image
