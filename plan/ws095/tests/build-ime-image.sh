#!/bin/sh
# ws095: builds the lean zdesktop guest image with the input method (plan/ws095/tests/config-amd64-ime.mk), with the
# guest harness's files, the wallpaper when made (build/ws035-wallpaper, not in git) and App Home's list of the
# demonstration (plan/ws035/demo/apps.conf), like plan/ws089/tests/build-settings-image.sh.
#
#   plan/ws095/tests/build-ime-image.sh [BUILD [DISTDIR]]   (default build/ws095/img; DISTDIR: where the dictionary's
#                                                             archive is fetched to, default the shared build/distfiles)
# Copyright (C) 2026 Awe Morris; SPDX-License-Identifier: Zlib
set -eu
cd "$(dirname -- "$0")/../../.."
. plan/tools/guest/jobs.sh  # ZEDBSD_JOBS, the parallel jobs (default 16)
build=${1:-build/ws095/img}
distdir=${2:-}
extra=$(python3 plan/tools/guest/guest.py extra-files | sed -n "s/^ZEDBSD_TEST_EXTRA_FILES='\(.*\)'$/\1/p")
[ -n "$extra" ] || { echo "build-ime-image: no guest files (plan/tools/guest/guest.py keys?)"; exit 1; }
[ -f build/ws035-wallpaper/wallpaper.ppm ] && extra="$extra --file /usr/share/keiland/wallpaper.ppm=build/ws035-wallpaper/wallpaper.ppm"
extra="$extra --file /etc/keiland/apps.conf=plan/ws035/demo/apps.conf"
set -- make -j"$ZEDBSD_JOBS" ZEDBSD_CONFIG=plan/ws095/tests/config-amd64-ime.mk BUILD="$build" "ZEDBSD_TEST_EXTRA_FILES=$extra"
[ -n "$distdir" ] && set -- "$@" "ZEDBSD_EXTERNAL_DISTDIR=$distdir"
exec "$@" disk-image
