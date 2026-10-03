#!/bin/sh
# ws079-p006: builds the lean Venus guest image of the File Manager tests (plan/tools/files/config-amd64-files.mk,
# which has PDF Viewer and libpdf since ws079-p006) with the guest harness's files, the fonts, the wallpaper, and
# the test documents of run-pdf-render.sh (the Notes-like and the operator documents) in /usr/share/pdfviewer-tests.
#   [DRY=-n] plan/ws079/tests/build-pdfviewer-image.sh BUILD
# Needs build/ws079-p006-host/notes.pdf and ops.pdf (sh plan/ws079/tests/run-pdf-render.sh).
# Copyright (C) 2026 Awe Morris; SPDX-License-Identifier: Zlib
set -eu
cd "$(dirname -- "$0")/../../.."
. plan/tools/guest/jobs.sh  # ZEDBSD_JOBS, the parallel jobs (default 16)
build=$1

extra=$(python3 plan/tools/guest/guest.py extra-files | sed -n "s/^ZEDBSD_TEST_EXTRA_FILES='\(.*\)'$/\1/p")
[ -n "$extra" ] || { echo "build-pdfviewer-image: no guest files (plan/tools/guest/guest.py keys?)"; exit 1; }
[ -f build/ws035-fonts/Inter.ttf ] && extra="$extra --file /usr/share/fonts/keiland.ttf=build/ws035-fonts/Inter.ttf"
[ -f build/ws035-fonts/OFL.txt ] && extra="$extra --file /usr/share/fonts/keiland-OFL.txt=build/ws035-fonts/OFL.txt"
[ -f build/ws035-fonts/JetBrainsMono-Regular.ttf ] && extra="$extra --file /usr/share/fonts/keiland-mono.ttf=build/ws035-fonts/JetBrainsMono-Regular.ttf"
[ -f build/ws035-fonts/JetBrainsMono-OFL.txt ] && extra="$extra --file /usr/share/fonts/keiland-mono-OFL.txt=build/ws035-fonts/JetBrainsMono-OFL.txt"
[ -f build/ws035-wallpaper/wallpaper.ppm ] && extra="$extra --file /usr/share/keiland/wallpaper.ppm=build/ws035-wallpaper/wallpaper.ppm"
extra="$extra --file /usr/share/files-tests/make-home.sh=plan/tools/files/make-home.sh"
extra="$extra --file /usr/share/pdfviewer-tests/notes.pdf=build/ws079-p006-host/notes.pdf"
extra="$extra --file /usr/share/pdfviewer-tests/ops.pdf=build/ws079-p006-host/ops.pdf"
make ${DRY:-} -j"$ZEDBSD_JOBS" ZEDBSD_CONFIG=plan/tools/files/config-amd64-files.mk BUILD="$build" \
    "ZEDBSD_TEST_EXTRA_FILES=$extra" disk-image
