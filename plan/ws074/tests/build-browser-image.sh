#!/bin/sh
# ws074: builds the zdesktop guest image with browser (plan/ws074/tests/config-amd64-browser.mk),
# the guest harness's files, the fonts and the wallpaper, like plan/tools/files/build-files-image.sh.
# The fonts and the wallpaper are not in git: build/ws035-fonts (Inter, JetBrains Mono, Droid Sans
# Fallback) and build/ws035-wallpaper; a worktree links them from the main checkout's build/.
# The browser's test pages (plan/ws074/tests/pages/) go to /usr/share/browser-tests/, the image test page and
# its pictures (make-test-images.py) to /usr/share/browser-images/.
#
#   plan/ws074/tests/build-browser-image.sh [BUILD]     (default build/amd64)
# Copyright (C) 2026 Awe Morris; SPDX-License-Identifier: Zlib
set -eu
cd "$(dirname -- "$0")/../../.."
. plan/tools/guest/jobs.sh  # ZEDBSD_JOBS, the parallel jobs (default 16)
build=${1:-build/amd64}
extra=$(python3 plan/tools/guest/guest.py extra-files | sed -n "s/^ZEDBSD_TEST_EXTRA_FILES='\(.*\)'$/\1/p")
[ -n "$extra" ] || { echo "build-browser-image: no guest files (plan/tools/guest/guest.py keys?)"; exit 1; }
fonts=build/ws035-fonts
[ -f $fonts/Inter.ttf ] && extra="$extra --file /usr/share/fonts/keiland.ttf=$fonts/Inter.ttf"
[ -f $fonts/OFL.txt ] && extra="$extra --file /usr/share/fonts/keiland-OFL.txt=$fonts/OFL.txt"
[ -f $fonts/JetBrainsMono-Regular.ttf ] && extra="$extra --file /usr/share/fonts/keiland-mono.ttf=$fonts/JetBrainsMono-Regular.ttf"
[ -f $fonts/JetBrainsMono-OFL.txt ] && extra="$extra --file /usr/share/fonts/keiland-mono-OFL.txt=$fonts/JetBrainsMono-OFL.txt"
[ -f $fonts/DroidSansFallbackFull.ttf ] && extra="$extra --file /usr/share/fonts/keiland-fallback.ttf=$fonts/DroidSansFallbackFull.ttf"
[ -f $fonts/DroidSansFallback-LICENSE.txt ] && extra="$extra --file /usr/share/fonts/keiland-fallback-LICENSE.txt=$fonts/DroidSansFallback-LICENSE.txt"
[ -f build/ws035-wallpaper/wallpaper.ppm ] && extra="$extra --file /usr/share/keiland/wallpaper.ppm=build/ws035-wallpaper/wallpaper.ppm"
# The File Manager tests' home maker, as the files image has it (the desktop regressions run on this image too).
extra="$extra --file /usr/share/files-tests/make-home.sh=plan/tools/files/make-home.sh"
if [ -d plan/ws074/tests/pages ]; then
	for page in plan/ws074/tests/pages/*; do
		[ -f "$page" ] && extra="$extra --file /usr/share/browser-tests/$(basename "$page")=$page"
	done
fi
# The image test page and its pictures (ws074-p021), made here and not in git, go to /usr/share/browser-images/.
python3 plan/ws074/tests/make-test-images.py >/dev/null
for picture in build/ws074-images/*; do
	[ -f "$picture" ] && extra="$extra --file /usr/share/browser-images/$(basename "$picture")=$picture"
done
exec make -j"$ZEDBSD_JOBS" ZEDBSD_CONFIG=plan/ws074/tests/config-amd64-browser.mk BUILD="$build" \
    "ZEDBSD_TEST_EXTRA_FILES=$extra" disk-image
