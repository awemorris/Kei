#!/bin/sh
# Builds the zdesktop guest image for the Venus tests (build/ws035-sq): the guest harness's files
# (SSH keys, net.conf) and, when they are present, the files kept out of git: the fonts
# (build/ws035-fonts/: Inter for zdesktop, JetBrains Mono for terminal, both OFL; Droid Sans
# Fallback Full for the characters Inter lacks (Japanese), Apache-2.0, from Debian's fonts-droid-fallback,
# /usr/share/fonts/truetype/droid/, with its copyright file as DroidSansFallback-LICENSE.txt) and the
# wallpaper (build/ws035-wallpaper/wallpaper.ppm, the user's picture).
#
#   plan/ws073/tests/build-image-noclang.sh [BUILD]
# ws073-p026: the same image without the clang and libcxx packages (config-amd64-zdesktop-noclang.mk).
# Copyright (C) 2026 Awe Morris; SPDX-License-Identifier: Zlib
set -eu
cd "$(dirname -- "$0")/../../.."
. plan/tools/guest/jobs.sh  # ZEDBSD_JOBS, the parallel jobs (default 16)
build=${1:-build/amd64}
extra=$(python3 plan/tools/guest/guest.py extra-files | sed -n "s/^ZEDBSD_TEST_EXTRA_FILES='\(.*\)'$/\1/p")
[ -n "$extra" ] || { echo "build-zdesktop-image: no guest files (plan/tools/guest/guest.py keys?)"; exit 1; }
[ -f build/ws035-fonts/Inter.ttf ] && extra="$extra --file /usr/share/fonts/keiland.ttf=build/ws035-fonts/Inter.ttf"
[ -f build/ws035-fonts/OFL.txt ] && extra="$extra --file /usr/share/fonts/keiland-OFL.txt=build/ws035-fonts/OFL.txt"
[ -f build/ws035-fonts/JetBrainsMono-Regular.ttf ] && extra="$extra --file /usr/share/fonts/keiland-mono.ttf=build/ws035-fonts/JetBrainsMono-Regular.ttf"
[ -f build/ws035-fonts/JetBrainsMono-OFL.txt ] && extra="$extra --file /usr/share/fonts/keiland-mono-OFL.txt=build/ws035-fonts/JetBrainsMono-OFL.txt"
[ -f build/ws035-fonts/DroidSansFallbackFull.ttf ] && extra="$extra --file /usr/share/fonts/keiland-fallback.ttf=build/ws035-fonts/DroidSansFallbackFull.ttf"
[ -f build/ws035-fonts/DroidSansFallback-LICENSE.txt ] && extra="$extra --file /usr/share/fonts/keiland-fallback-LICENSE.txt=build/ws035-fonts/DroidSansFallback-LICENSE.txt"
[ -f build/ws035-wallpaper/wallpaper.ppm ] && extra="$extra --file /usr/share/keiland/wallpaper.ppm=build/ws035-wallpaper/wallpaper.ppm"
exec make -j"$ZEDBSD_JOBS" ZEDBSD_CONFIG=plan/ws073/tests/config-amd64-zdesktop-noclang.mk BUILD="$build" \
    "ZEDBSD_TEST_EXTRA_FILES=$extra" disk-image
