#!/bin/sh
# ws081-p018: builds the demonstration image for the QEMU on Windows (config-amd64-demo-win.mk) like
# plan/ws075/demo/build-demo-image.sh: App Home's list, the wallpapers, the demonstration's accounts (kei, root), the
# guest harness's key for root, and touchlog (built first, against the toolchain's sysroot).
#   plan/ws081/tests/build-demo-win.sh [BUILD]     (default build/ws081-demo-win; the image is BUILD/hdd-image.img)
# Copyright (C) 2026 Awe Morris; SPDX-License-Identifier: Zlib
set -eu
cd "$(dirname -- "$0")/../../.."
. plan/tools/guest/jobs.sh  # ZEDBSD_JOBS, the parallel jobs (default 16)
build=${1:-build/ws081-demo-win}
mkdir -p "$build"
extra="--file /etc/keiland/apps.conf=plan/ws035/demo/apps.conf"
[ -f build/ws035-wallpaper/wallpaper.ppm ] && extra="$extra --file /usr/share/keiland/wallpaper.ppm=build/ws035-wallpaper/wallpaper.ppm"
python3 userland/desktop/wallpapers/generate.py "$build/wallpapers" >/dev/null
for picture in "$build"/wallpapers/*.ppm; do
	extra="$extra --file /usr/share/keiland/wallpapers/$(basename "$picture")=$picture"
done
accounts=$build/demo-accounts
plan/ws035/demo/demo-accounts.sh "$accounts"
key=plan/tmp/guest/id_ed25519.pub
[ -f "$key" ] && extra="$extra --file /root/.ssh/authorized_keys=$key --mode /root/.ssh/authorized_keys=0600 --mode /root/.ssh=0700"
extra="$extra --file /etc/passwd=$accounts/passwd --file /etc/group=$accounts/group --file /etc/shadow=$accounts/shadow"
# touchlog first, against the toolchain's sysroot (build/amd64/sysroot, the one the packages are linked with).
sh plan/ws081/tests/build-touchlog.sh build/amd64
make -j"$ZEDBSD_JOBS" ZEDBSD_CONFIG=plan/ws081/tests/config-amd64-demo-win.mk BUILD="$build" \
	"ZEDBSD_TEST_EXTRA_FILES=$extra" ZEDBSD_TEST_IMAGE_TAG=demo-win disk-image
echo "demo image: $build/hdd-image.img"
