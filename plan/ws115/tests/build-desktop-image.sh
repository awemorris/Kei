#!/bin/sh
# ws115-p009: the compositor criteria image (plan/ws099/tests/build-criteria-image.sh: the graphical login image
# with Terminal, Files, wltest and the protocol probes) with the xdg-shell probe (plan/ws115/tests/xdg-probe.sh) added.
#
#   plan/ws115/tests/build-desktop-image.sh BUILD PROBE-EXTRA [MAKE-VARIABLE...]
#
# PROBE-EXTRA is the ZEDBSD_TEST_EXTRA_FILES value xdg-probe.sh printed; the make variables (such as
# ZEDBSD_LLVM_SOURCE=...) are passed on.
# Copyright (C) 2026 Awe Morris; SPDX-License-Identifier: Zlib
set -eu
cd "$(dirname -- "$0")/../../.."
build=$1
probe=$2
shift 2
extra=$(python3 plan/tools/guest/guest.py extra-files | sed -n "s/^ZEDBSD_TEST_EXTRA_FILES='\(.*\)'$/\1/p")
[ -n "$extra" ] || { echo "build-desktop-image: no guest files (plan/tools/guest/guest.py keys?)"; exit 1; }
extra="$extra --file /usr/share/fonts/keiland.ttf=userland/desktop/fonts/Inter.ttf"
extra="$extra --file /usr/share/fonts/keiland-mono.ttf=userland/desktop/fonts/JetBrainsMono-Regular.ttf"
extra="$extra --file /usr/share/fonts/keiland-fallback.ttf=userland/desktop/fonts/DroidSansFallbackFull.ttf"
extra="$extra --file /usr/share/files-tests/make-home.sh=plan/tools/files/make-home.sh"
python3 userland/desktop/wallpapers/generate.py "$build/wallpapers" >/dev/null
for picture in "$build"/wallpapers/*.ppm; do
	extra="$extra --file /usr/share/keiland/wallpapers/$(basename "$picture")=$picture"
done
extra="$extra $probe"
exec make -j"$(nproc)" ZEDBSD_CONFIG=plan/ws099/tests/config-amd64-criteria.mk BUILD="$build" \
    "ZEDBSD_TEST_EXTRA_FILES=$extra" ZEDBSD_TEST_IMAGE_TAG=wsp009 "$@" disk-image
