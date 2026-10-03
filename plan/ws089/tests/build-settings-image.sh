#!/bin/sh
# ws089: builds the lean zdesktop guest image with Settings (plan/ws089/tests/config-amd64-settings.mk), with the
# guest harness's files, the wallpaper (build/ws035-wallpaper, not in git) and App Home's list of the demonstration
# (plan/ws035/demo/apps.conf), like plan/tools/files/build-files-image.sh.  The fonts come with the compositor's package.
#
#   plan/ws089/tests/build-settings-image.sh [BUILD]     (default build/amd64)
#   SETTINGS_CONFIG=plan/ws089/tests/config-amd64-settings-ime.mk plan/ws089/tests/build-settings-image.sh BUILD
#                                                       (another config, such as the one with the input method)
# Copyright (C) 2026 Awe Morris; SPDX-License-Identifier: Zlib
set -eu
cd "$(dirname -- "$0")/../../.."
. plan/tools/guest/jobs.sh  # ZEDBSD_JOBS, the parallel jobs (default 16)
build=${1:-build/amd64}
extra=$(python3 plan/tools/guest/guest.py extra-files | sed -n "s/^ZEDBSD_TEST_EXTRA_FILES='\(.*\)'$/\1/p")
[ -n "$extra" ] || { echo "build-settings-image: no guest files (plan/tools/guest/guest.py keys?)"; exit 1; }
[ -f build/ws035-wallpaper/wallpaper.ppm ] && extra="$extra --file /usr/share/keiland/wallpaper.ppm=build/ws035-wallpaper/wallpaper.ppm"
extra="$extra --file /etc/keiland/apps.conf=plan/ws035/demo/apps.conf"
# ws089-p009: the wallpapers Settings offers, as the demonstration image has them (userland/desktop/wallpapers/generate.py).
python3 userland/desktop/wallpapers/generate.py "$build/wallpapers" >/dev/null
for picture in "$build"/wallpapers/*.ppm; do
	extra="$extra --file /usr/share/keiland/wallpapers/$(basename "$picture")=$picture"
done
exec make -j"$ZEDBSD_JOBS" ZEDBSD_CONFIG="${SETTINGS_CONFIG:-plan/ws089/tests/config-amd64-settings.mk}" BUILD="$build" \
    "ZEDBSD_TEST_EXTRA_FILES=$extra" disk-image
