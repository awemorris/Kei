#!/bin/sh
# ws005-p020: builds the Venus desktop image with the RTL8822B firmware (config-venus-rtl.mk) and the guest
# harness's files (SSH keys), the wallpaper and App Home's list, like plan/ws089/tests/build-settings-image.sh.
# Graphical boot (login=graphical), so sessiond starts the desktop with kei logged in.
#   sh plan/ws005/phase020/build-rtl-image.sh BUILD
# Copyright (C) 2026 Awe Morris; SPDX-License-Identifier: Zlib
set -eu
cd "$(dirname -- "$0")/../../.."
build=${1:?usage: build-rtl-image.sh BUILD}
extra=$(python3 plan/tools/guest/guest.py extra-files | sed -n "s/^ZEDBSD_TEST_EXTRA_FILES='\(.*\)'$/\1/p")
[ -n "$extra" ] || { echo "build-rtl-image: no guest files"; exit 1; }
[ -f build/ws035-wallpaper/wallpaper.ppm ] && extra="$extra --file /usr/share/keiland/wallpaper.ppm=build/ws035-wallpaper/wallpaper.ppm"
extra="$extra --file /etc/keiland/apps.conf=plan/ws035/demo/apps.conf"
exec make -j"$(nproc)" ZEDBSD_CONFIG=plan/ws005/phase020/config-venus-rtl.mk BUILD="$build" ZEDBSD_GRAPHICAL_BOOT=y \
    "ZEDBSD_TEST_EXTRA_FILES=$extra" disk-image
