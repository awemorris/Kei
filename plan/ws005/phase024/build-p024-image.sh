#!/bin/sh
# ws005-p024: builds the 5330 passthrough test image for the Wi-Fi store sessions.
# Copyright (C) 2026 Awe Morris; SPDX-License-Identifier: Zlib
#
# The AX211 desktop image of ws005-p019 (plan/ws075/demo/build-demo-image.sh with
# plan/ws004/tests/config-ax211-desktop.mk, passthrough), with the same demonstration files, and for this test:
# no automatic login (an empty /etc/keiland/autologin: the greeter shows at boot, so a boot without a session can
# be watched), and the p024watch service (image/p024-watch.sh), which records the Wi-Fi state from the boot on
# and stages the system's store from kei's on request.  No key is in the image.
#   sh plan/ws005/phase024/build-p024-image.sh [BUILD]        (default build/p1-wdesk24)
set -eu
cd "$(dirname -- "$0")/../../.."
build=${1:-build/p1-wdesk24}
here=plan/ws005/phase024/image
extra="--file /etc/keiland/apps.conf=plan/ws035/demo/apps.conf"
[ -f build/ws035-wallpaper/wallpaper-1080.ppm ] && extra="$extra --file /usr/share/keiland/wallpaper.ppm=build/ws035-wallpaper/wallpaper-1080.ppm"
mkdir -p "$build"
python3 userland/desktop/wallpapers/generate.py "$build/wallpapers" >/dev/null
for picture in "$build"/wallpapers/*.ppm; do
	extra="$extra --file /usr/share/keiland/wallpapers/$(basename "$picture")=$picture"
done
accounts=$build/demo-accounts
plan/ws035/demo/demo-accounts.sh "$accounts"
extra="$extra --file /etc/passwd=$accounts/passwd --file /etc/group=$accounts/group --file /etc/shadow=$accounts/shadow"
extra="$extra --file /etc/keiland/autologin=$here/autologin-none --file /etc/rc.conf=$here/rc.conf"
extra="$extra --file /etc/service.d/p024watch=$here/p024watch.service --file /etc/p024-watch.sh=$here/p024-watch.sh"
extra="$extra --mode /etc/p024-watch.sh=0755"
make -j"$(nproc)" ZEDBSD_CONFIG=plan/ws004/tests/config-ax211-desktop.mk BUILD="$build" I915_TEST_VBT=y \
	"ZEDBSD_TEST_EXTRA_FILES=$extra" ZEDBSD_TEST_IMAGE_TAG=demo-hdmi disk-image
echo "p024 image: $build/hdd-image.img"
