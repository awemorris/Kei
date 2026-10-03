#!/bin/sh
# ws101-p015: builds the demonstration image for the 5330's passthrough (as plan/ws075/demo/build-demo-image.sh BUILD
# passthrough does: the accounts, the wallpapers, App Home's apps.conf, the guest harness's key) from
# plan/ws101/tests/demo/config.mk, with /bin/noct taken from NOCT (an accelerator-enabled build's bin/noct, by default
# the main checkout's demonstration build) instead of building Noct here.
#
#   plan/ws101/tests/demo/build-s13-image.sh [BUILD]      (default build/ws101-p015-demo; NOCT)
# Copyright (C) 2026 Awe Morris; SPDX-License-Identifier: Zlib
set -eu
cd "$(dirname -- "$0")/../../../.."
. plan/tools/guest/jobs.sh  # ZEDBSD_JOBS, the parallel jobs (default 16)
build=${1:-build/ws101-p015-demo}
noct=${NOCT:-/home/awe/zedBSD-rpi4/build/demo-lcd9/bin/noct}
[ -x "$noct" ] || { echo "build-s13-image: no accelerator-enabled noct at $noct"; exit 1; }
extra="--file /etc/keiland/apps.conf=plan/ws035/demo/apps.conf --file /bin/noct=$noct --mode /bin/noct=0755"
wallpaper=/home/awe/zedBSD-rpi4/build/ws035-wallpaper/wallpaper-1080.ppm
[ -f "$wallpaper" ] && extra="$extra --file /usr/share/keiland/wallpaper.ppm=$wallpaper"
mkdir -p "$build"
python3 userland/desktop/wallpapers/generate.py "$build/wallpapers" >/dev/null
for picture in "$build"/wallpapers/*.ppm; do
	extra="$extra --file /usr/share/keiland/wallpapers/$(basename "$picture")=$picture"
done
accounts=$build/demo-accounts
plan/ws035/demo/demo-accounts.sh "$accounts"
key=plan/tmp/guest/id_ed25519.pub
[ -f "$key" ] && extra="$extra --file /root/.ssh/authorized_keys=$key --mode /root/.ssh/authorized_keys=0600 --mode /root/.ssh=0700"
extra="$extra --file /etc/passwd=$accounts/passwd --file /etc/group=$accounts/group --file /etc/shadow=$accounts/shadow"
make -j"$ZEDBSD_JOBS" ZEDBSD_CONFIG=plan/ws101/tests/demo/config.mk BUILD="$build" I915_TEST_VBT=y \
	"ZEDBSD_TEST_EXTRA_FILES=$extra" ZEDBSD_TEST_IMAGE_TAG=ws101-demo disk-image
echo "s13 demo image: $build/hdd-image.img"
