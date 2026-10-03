#!/bin/sh
# ws134-p003: draws the System Monitor's screen on the host (preview.c and preview.py) into a PNG, for looking at the
# layout and the 3D parts without the guest.  Needs the Linux Keiland build's libkeiui and libtruetype
# (KEILAND_LINUX_BUILD, default build/p2-keiland-linux) and the fonts of build/ws035-fonts.
#   plan/ws134/tests/host/preview.sh OUT.png [WIDTH HEIGHT TIME_MS SOURCE [POINTER_X POINTER_Y]]
#     SOURCE: replay:FILE or sim:SEED[:CPUS:GPUS] (default sim:3:8:1 at 120000 ms, 1200x760)
# Copyright (C) 2026 Awe Morris; SPDX-License-Identifier: Zlib
set -eu
cd "$(dirname -- "$0")/../../../.."
out=$1
linux=${KEILAND_LINUX_BUILD:-build/p2-keiland-linux}
work=build/ws134-preview
mkdir -p "$work"
m=userland/desktop/monitor
cc -std=gnu17 -D_GNU_SOURCE -O1 -Wall -Wextra -Werror -Wno-format-truncation -DKEILAND_DATADIR='"/usr/share"' -I. -Iuserland/desktop/keiland -I"$linux/include" -I"$m" \
    plan/ws134/tests/host/preview.c "$m/source.c" "$m/history.c" "$m/rules.c" "$m/format.c" "$m/atlas.c" "$m/draw.c" "$m/scene.c" "$m/space.c" \
    -L"$linux/lib" -Wl,-rpath,"$(pwd)/$linux/lib" -lkeiui -lkeiland -ltruetype -lm -o "$work/preview"
LD_LIBRARY_PATH="$(pwd)/$linux/lib${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}" "$work/preview" "$work/scene.bin" "${2:-1200}" "${3:-760}" "${4:-120000}" build/ws035-fonts/Inter.ttf build/ws035-fonts/JetBrainsMono-Regular.ttf \
    "${5:-sim:3:8:1}" ${6:-} ${7:-}
python3 plan/ws134/tests/host/preview.py "$work/scene.bin" "$out"
echo "preview: $out"
