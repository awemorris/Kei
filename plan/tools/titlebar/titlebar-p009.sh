#!/bin/sh
# ws070-p009: the glass look's glyph cache (UTF-8, the fallback font) on the Venus guest (the lean
# image with build/ws071-fonts' fallback font).  zdesktop --glass shows /bin/titlebar-probe --show with a
# Japanese title:
#  1. zdesktop made the atlas's cache (GLASS atlas), opened the fallback font on the first character
#     the first font lacks (GLASS fallback font ... faces=2; lazily since ws070-p010), and rendered the
#     title's Japanese characters into it (GLASS glyph codepoint=U+65E5 ... face=1).
#  2. floating.png: the title in the floating titlebar; docked.png: docked by a double click on the
#     title, the title in the system bar.
#
#   plan/tools/files/files-guest.sh start     (the guest must be up; GUEST_RUNTIME as for it)
#   plan/tools/titlebar/titlebar-p009.sh [OUTDIR]
# Copyright (C) 2026 Awe Morris; SPDX-License-Identifier: Zlib
set -u
cd "$(dirname -- "$0")/../../.."
GUEST_RUNTIME="${GUEST_RUNTIME:-$(pwd)/build/ws071-run}"
export GUEST_RUNTIME
out=${1:-build/ws070-p009}
mkdir -p "$out"
guest() { timeout 90 python3 plan/tools/guest/guest.py run "$1" 2>&1; }
. plan/tools/guest/zwl-clients.sh
check() { python3 plan/ws035/tests/zdesktop-check.py "$@" --runtime "$GUEST_RUNTIME"; }
pointer() { python3 plan/ws035/tests/qmp-pointer.py "$GUEST_RUNTIME/qmp.sock" "$@"; }
stop_all='for p in $(ps -A -o pid,args | grep -E "[w]ayland( |$)|[t]itlebar-probe" | awk "{print \$1}"); do kill $p; done; i=0; while ps -A -o args | grep -qE "[w]ayland( |$)|[t]itlebar-probe" && [ $i -lt 50 ]; do sleep 0.2; i=$((i+1)); done'
status=0

# Fails the run unless a log has a line matching a pattern.
expect_log() {
	found=$(guest "grep -cE '$2' $1" | tail -1)
	if [ "${found:-0}" -gt 0 ] 2>/dev/null; then
		echo "log: $2 ok"
	else
		echo "log: $2 MISSING"
		status=1
	fi
}

guest "$stop_all" >/dev/null
guest 'export XDG_RUNTIME_DIR=/tmp; rm -f /tmp/wayland-0
picture=; [ -f /usr/share/keiland/wallpaper.ppm ] && picture=--wallpaper=/usr/share/keiland/wallpaper.ppm
/bin/wayland --timeout=300 --width=1280 --height=800 --glass $picture > /tmp/zdesktop.log 2>&1 </dev/null & sleep 4
/bin/titlebar-probe --show="日本語のタイトル — 表示の試験" --seconds=120 > /tmp/probe.log 2>&1 </dev/null & sleep 5; echo started' >/dev/null
zwl_app_clients
set -- $(guest "grep 'ZWL MAP client=$zc1 ' /tmp/zdesktop.log | tail -1" | sed -n 's/.* surface=\([0-9]*\) x=\([-0-9]*\) y=\([-0-9]*\).*/\1 \2 \3/p')
surface=${1:-0}; wx=${2:-0}; wy=${3:-0}
echo "probe: surface $surface at $wx,$wy"

# 1. The fonts, the cache and the Japanese glyphs.
expect_log /tmp/zdesktop.log 'ZWL GLASS atlas cache-top=[0-9]+ cells=[0-9]+ faces=1'
expect_log /tmp/zdesktop.log 'ZWL GLASS fallback font: path=[^ ]+ faces=2'
expect_log /tmp/zdesktop.log 'ZWL GLASS glyph codepoint=U\+65E5 size=[0-9]+ face=1 '
expect_log /tmp/probe.log 'TITLEBARPROBE show ready mode=menu'

# 2. The title floating, then docked (a double click on the title bar, above the body).
pointer move 1270 790 sleep 500
check "$out/floating.png" >/dev/null
pointer move $((wx + 300)) $((wy - 30)) sleep 300 down sleep 50 up sleep 80 down sleep 50 up sleep 1200
pointer move 1270 790 sleep 600
check "$out/docked.png" >/dev/null
expect_log /tmp/zdesktop.log "GLASS dock surface=$surface"

errors=$(guest "grep -c ERROR /tmp/zdesktop.log" | tail -1)
[ "${errors:-1}" = 0 ] && echo "zdesktop: no ERROR" || { echo "zdesktop: ERROR lines"; status=1; }
guest 'grep "ZWL GLASS" /tmp/zdesktop.log' > "$out/glass.log"
guest "$stop_all" >/dev/null
[ $status = 0 ] && echo "titlebar-p009: PASS" || echo "titlebar-p009: FAIL"
exit $status
