#!/bin/sh
# ws127-p001: helpers for walking through files by hand on the Venus guest (the lean image,
# plan/tools/files/build-files-image.sh; the guest started with plan/tools/files/files-guest.sh).
# Source it from the worktree root:
#
#   . plan/ws127/tests/walk-lib.sh
#   walk_start FOLDER [WIDTH HEIGHT]    zdesktop --glass 1280x800 and files on FOLDER of the sample home
#   walk_click X Y [BUTTON]             a click on the window body (X, Y from its top left)
#   walk_double X Y                     a double click on the window body
#   walk_keys TEXT...                   keys (plan/ws035/tests/qmp-keys.py words: <ctrl-c>, text, \n)
#   walk_shot NAME                      the screen into $WALK_OUT/NAME.png
#   walk_log PATTERN                    counts the lines of /tmp/f.log matching PATTERN (and prints them)
#   walk_run COMMAND                    a command in the guest
#
# Judgement uses files' own log (/tmp/f.log) and the screen, never the guest's console.
# Copyright (C) 2026 Awe Morris; SPDX-License-Identifier: Zlib
GUEST_RUNTIME="${GUEST_RUNTIME:-$(pwd)/build/p4-run}"
export GUEST_RUNTIME
WALK_OUT=${WALK_OUT:-build/p4-files-out/walk}
mkdir -p "$WALK_OUT"

walk_run() { timeout 120 python3 plan/tools/guest/guest.py run "$1" 2>&1; }
ZWL_RUN=walk_run
. plan/tools/guest/zwl-clients.sh

walk_stop_all='service stop greeter >/dev/null 2>&1; for p in $(ps -A -o pid,args | grep -E "[w]ayland( |$)|[f]iles|[t]erminal|[i]mageview|[t]extedit|[p]dfviewer" | awk "{print \$1}"); do kill $p; done; i=0; while ps -A -o args | grep -qE "[w]ayland( |$)|[f]iles" && [ $i -lt 50 ]; do sleep 0.2; i=$((i+1)); done'

walk_start() {
	folder=$1
	width=${2:-1000}
	height=${3:-640}
	walk_run "$walk_stop_all" >/dev/null
	walk_run 'rm -f /tmp/wayland-0 /tmp/files.clipboard; [ -d /tmp/fhome ] || sh /usr/share/files-tests/make-home.sh /tmp/fhome >/dev/null' >/dev/null
	walk_run "export XDG_RUNTIME_DIR=/tmp
picture=; [ -f /usr/share/keiland/wallpaper.ppm ] && picture=--wallpaper=/usr/share/keiland/wallpaper.ppm
/bin/wayland --timeout=1800 --width=1280 --height=800 --glass \$picture > /tmp/zdesktop.log 2>&1 </dev/null & sleep 4
HOME=/tmp/fhome /bin/files --token=f1 --timeout-s=1700 --width=$width --height=$height '$folder' > /tmp/f.log 2>&1 </dev/null & echo started" >/dev/null
	i=0
	while [ $i -lt 30 ]; do
		zwl_app_clients
		set -- $(walk_run "grep 'ZWL MAP client=$zc1 ' /tmp/zdesktop.log | tail -1" | sed -n 's/.* surface=\([0-9]*\) x=\([-0-9]*\) y=\([-0-9]*\).*/\1 \2 \3/p')
		[ -n "${1:-}" ] && break
		sleep 1
		i=$((i+1))
	done
	WALK_X=${2:-0}
	WALK_Y=${3:-0}
	echo "walk: files at $WALK_X,$WALK_Y"
}

walk_pointer() { python3 plan/tools/files/qmp-input.py "$GUEST_RUNTIME/qmp.sock" "$@"; }

walk_click() {
	walk_pointer move $((WALK_X + $1 - 2)) $((WALK_Y + $2)) sleep 150 move $((WALK_X + $1)) $((WALK_Y + $2)) sleep 200 \
	    down ${3:-left} sleep 60 up ${3:-left} sleep 600
}

walk_double() {
	walk_pointer move $((WALK_X + $1)) $((WALK_Y + $2)) sleep 200 down sleep 40 up sleep 80 down sleep 40 up sleep 900
}

walk_keys() { python3 plan/ws035/tests/qmp-keys.py "$GUEST_RUNTIME/qmp.sock" "$@"; sleep 0.8; }

walk_shot() {
	walk_pointer move 1270 790 sleep 400
	python3 plan/ws035/tests/zdesktop-check.py "$WALK_OUT/$1.png" --runtime "$GUEST_RUNTIME" >/dev/null
	echo "walk: shot $WALK_OUT/$1.png"
}

walk_log() {
	walk_run "grep -E '$1' /tmp/f.log | tail -5; grep -cE '$1' /tmp/f.log"
}
