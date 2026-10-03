#!/bin/sh
# ws035-p094〜: builds the lean zdesktop guest image with the graphical login's test greeter
# (plan/ws035/tests/config-amd64-login.mk), like plan/tools/files/build-files-image.sh (the guest harness's
# files, the fonts and the wallpaper, which are not in git).
#
#   plan/ws035/tests/build-login-image.sh [BUILD] [graphical] [TARGET...]     (default build/amd64, disk-image)
# With "graphical" the image boots graphically (config-amd64-graphical.mk: logo, kmsg=quiet, login=graphical);
# "graphical-network" adds the networkd stand-in (config-amd64-graphical-network.mk, ws035-p104).
# Copyright (C) 2026 Awe Morris; SPDX-License-Identifier: Zlib
set -eu
cd "$(dirname -- "$0")/../../.."
. plan/tools/guest/jobs.sh  # ZEDBSD_JOBS, the parallel jobs (default 16)
build=${1:-build/amd64}
variant=${2:-}
[ $# -ge 2 ] && shift 2 || shift $#
config=plan/ws035/tests/config-amd64-login.mk
[ "$variant" = graphical ] && config=plan/ws035/tests/config-amd64-graphical.mk
[ "$variant" = graphical-network ] && config=plan/ws035/tests/config-amd64-graphical-network.mk
extra=$(python3 plan/tools/guest/guest.py extra-files | sed -n "s/^ZEDBSD_TEST_EXTRA_FILES='\(.*\)'$/\1/p")
[ -n "$extra" ] || { echo "build-login-image: no guest files (plan/tools/guest/guest.py keys?)"; exit 1; }
[ -f build/ws035-fonts/Inter.ttf ] && extra="$extra --file /usr/share/fonts/keiland.ttf=build/ws035-fonts/Inter.ttf"
[ -f build/ws035-fonts/OFL.txt ] && extra="$extra --file /usr/share/fonts/keiland-OFL.txt=build/ws035-fonts/OFL.txt"
[ -f build/ws035-fonts/JetBrainsMono-Regular.ttf ] && extra="$extra --file /usr/share/fonts/keiland-mono.ttf=build/ws035-fonts/JetBrainsMono-Regular.ttf"
[ -f build/ws035-fonts/JetBrainsMono-OFL.txt ] && extra="$extra --file /usr/share/fonts/keiland-mono-OFL.txt=build/ws035-fonts/JetBrainsMono-OFL.txt"
[ -f build/ws035-wallpaper/wallpaper.ppm ] && extra="$extra --file /usr/share/keiland/wallpaper.ppm=build/ws035-wallpaper/wallpaper.ppm"
extra="$extra --file /usr/share/files-tests/make-home.sh=plan/tools/files/make-home.sh"
[ $# -eq 0 ] && set -- disk-image
exec make -j"$ZEDBSD_JOBS" ZEDBSD_CONFIG=$config BUILD="$build" \
    "ZEDBSD_TEST_EXTRA_FILES=$extra" "$@"
