#!/bin/sh
# ws035-p088: drag and drop's "ask" action and a drop of files on the terminal, on the Venus guest (the lean image,
# plan/tools/files/build-files-image.sh).  zdesktop --glass at 1280x800; files A (Desktop, left) and B
# (Documents, right), 600x560, on the sample home:
#  1. ask.png: Logo.png dragged from A to B with Alt held: zdesktop chooses "ask" (drag action ... action=4); on the
#     drop B shows its choice (a context menu: Move Here, Copy Here, Link Here, Cancel) at the drop's place;
#     Copy Here copies the file (it is in both folders; B finishes with the copy).
#  2. Screenshot.png dragged the same way and Cancel chosen: B gives the drop up (DROP ask cancel), zdesktop
#     cancels the source (drag unfinished), A's drag ends as not dropped; nothing moves.
#  3. term-drop.png: terminal opened on the right (over B); Screenshot.png dragged from A onto it: the
#     terminal takes the file names (accept mime=text/uri-list) and types the quoted path into its shell (ZTERM
#     DROP bytes=N uris=1); the file stays (a copy).
#
#   plan/tools/files/files-guest.sh start     (the guest must be up)
#   plan/ws035/tests/zdesktop-p088.sh [OUTDIR]
# Copyright (C) 2026 Awe Morris; SPDX-License-Identifier: Zlib
set -u
cd "$(dirname -- "$0")/../../.."
GUEST_RUNTIME="${GUEST_RUNTIME:-$(pwd)/build/ws071-run}"
export GUEST_RUNTIME
out=${1:-build/ws035-p088}
mkdir -p "$out"
guest() { timeout 90 python3 plan/tools/guest/guest.py run "$1" 2>&1; }
. plan/tools/guest/zwl-clients.sh
check() { python3 plan/ws035/tests/zdesktop-check.py "$@" --runtime "$GUEST_RUNTIME"; }
pointer() { python3 plan/ws035/tests/qmp-pointer.py "$GUEST_RUNTIME/qmp.sock" "$@"; }
input() { python3 plan/tools/files/qmp-input.py "$GUEST_RUNTIME/qmp.sock" "$@"; }
stop_all='service stop greeter >/dev/null 2>&1; for p in $(ps -A -o pid,args | grep -E "[w]ayland( |$)|[f]iles|[t]erminal" | awk "{print \$1}"); do kill $p; done; i=0; while ps -A -o args | grep -qE "[w]ayland( |$)|[f]iles|[t]erminal" && [ $i -lt 50 ]; do sleep 0.2; i=$((i+1)); done'
status=0

# Fails the run unless a log has a line matching a pattern (within a few seconds).
expect_log() {
	tries=0
	found=0
	while [ $tries -lt 6 ]; do
		found=$(guest "grep -cE '$2' $1" | tail -1)
		[ "${found:-0}" -gt 0 ] 2>/dev/null && break
		tries=$((tries + 1))
		sleep 1
	done
	if [ "${found:-0}" -gt 0 ] 2>/dev/null; then
		echo "log: $2 ok"
	else
		echo "log: $2 MISSING"
		status=1
	fi
}

# Fails the run unless a shell test holds in the guest.
expect_guest() {
	if [ "$(guest "$1 && echo yes" | tail -1)" = yes ]; then
		echo "guest: $2 ok"
	else
		echo "guest: $2 MISSING"
		status=1
	fi
}

window() {
	guest "grep 'ZWL MAP client=$(zwl_app_client $1) ' /tmp/zdesktop.log | tail -1" | sed -n 's/.* x=\([-0-9]*\) y=\([-0-9]*\).*/\1 \2/p'
}

# Drags between two screen points (optionally with a key held) and releases.
drag() {
	pointer move $(($1 - 2)) "$2" sleep 150 move "$1" "$2" sleep 300 down sleep 120 \
	    move $(($1 + 12)) $(($2 + 4)) sleep 100 move $((($1 + $3) / 2)) $((($2 + $4) / 2)) sleep 150 \
	    move $(($3 + 3)) $(($4 + 1)) sleep 150 move "$3" "$4" sleep 700 up sleep 1500
}
drag_alt() {
	input move $(($1 - 2)) "$2" sleep 150 move "$1" "$2" sleep 300 down sleep 120 \
	    move $(($1 + 12)) $(($2 + 4)) sleep 100 move $((($1 + $3) / 2)) $((($2 + $4) / 2)) sleep 150 \
	    hold alt sleep 150 move $(($3 + 3)) $(($4 + 1)) sleep 150 move "$3" "$4" sleep 700 up sleep 300 free alt sleep 1500
}
click() {
	pointer move $(($1 - 2)) "$2" sleep 150 move "$1" "$2" sleep 300 down sleep 60 up sleep "${3:-1200}"
}
shot() {
	pointer move 1270 790 sleep 400
	check "$out/$1" >/dev/null
}

# The middle (y) of the Nth row of the latest popup, and its left edge.
row_n() {
	guest "grep 'ZWL MENU row item=' /tmp/zdesktop.log | tail -5 | sed -n $1p" | sed -n 's/.* y=\([0-9]*\) height=\([0-9]*\).*/\1 \2/p' | { read y h; echo $(( ${y:-0} + ${h:-0} / 2 )); }
}
popup_x() {
	guest "grep 'MENU open ' /tmp/zdesktop.log | tail -1" | sed -n 's/.* x=\([0-9]*\) y=.*/\1/p'
}

guest "$stop_all" >/dev/null
guest 'rm -f /tmp/files.clipboard; rm -rf /tmp/fhome; sh /usr/share/files-tests/make-home.sh /tmp/fhome >/dev/null'
guest 'export XDG_RUNTIME_DIR=/tmp; rm -f /tmp/wayland-0
/bin/wayland --timeout=900 --width=1280 --height=800 --glass --wallpaper=/usr/share/keiland/wallpaper.ppm > /tmp/zdesktop.log 2>&1 </dev/null & for w in 1 2 3 4 5 6 7 8 9 10 11 12 13 14 15 16 17 18 19 20 21 22 23 24 25 26 27 28 29 30; do grep -q ZWL.READY /tmp/zdesktop.log 2>/dev/null && break; sleep 0.5; done; sleep 1
HOME=/tmp/fhome /bin/files --token=a --timeout-s=800 --width=600 --height=560 /tmp/fhome/Desktop > /tmp/a.log 2>&1 </dev/null & sleep 6; echo started' >/dev/null
set -- $(window 1)
ax=${1:-0}; ay=${2:-0}
drag $((ax + 60)) $((ay - 30)) $((ax + 60 - 320)) $((ay - 30))
ax=$((ax - 320))
guest 'export XDG_RUNTIME_DIR=/tmp; HOME=/tmp/fhome /bin/files --token=b --timeout-s=800 --width=600 --height=560 /tmp/fhome/Documents > /tmp/b.log 2>&1 </dev/null & sleep 6; echo started' >/dev/null
set -- $(window 2)
bx=${1:-0}; by=${2:-0}
drag $((bx + 60)) $((by - 30)) $((bx + 60 + 288)) $((by - 30))
bx=$((bx + 288))
echo "A at $ax,$ay  B at $bx,$by"

# 1. Alt: the choice, and Copy Here.
drag_alt $((ax + 330)) $((ay + 110)) $((bx + 420)) $((by + 420))
zwl_app_clients
expect_log /tmp/zdesktop.log "ZWL DATA drag action client=$zc2 action=4"
expect_log /tmp/b.log 'ZFILES DROP drop self=0 destination=/tmp/fhome/Documents action=4'
expect_log /tmp/b.log 'ZFILES CONTEXT-MENU open rows=5 '
sleep 1
shot ask.png
click $(( $(popup_x) + 60 )) "$(row_n 2)" 2000
expect_log /tmp/b.log 'ZFILES DROP operation=copy items=1 destination=/tmp/fhome/Documents first=/tmp/fhome/Desktop/Logo.png'
expect_log /tmp/zdesktop.log "ZWL DATA drag finish client=$zc2 action=1"
expect_guest '[ -f /tmp/fhome/Documents/Logo.png ] && [ -f /tmp/fhome/Desktop/Logo.png ]' 'Logo.png copied'

# 2. Alt again, and Cancel.
sleep 2
drag_alt $((ax + 440)) $((ay + 110)) $((bx + 420)) $((by + 420))
expect_log /tmp/b.log 'ZFILES CONTEXT-MENU open rows=5 '
sleep 1
click $(( $(popup_x) + 60 )) "$(row_n 5)" 2000
expect_log /tmp/b.log 'ZFILES DROP ask cancel'
expect_log /tmp/zdesktop.log "ZWL DATA drag unfinished client=$zc2"
expect_log /tmp/a.log 'ZFILES DRAG out done dropped=0'
expect_guest '[ -f /tmp/fhome/Desktop/Screenshot.png ] && [ ! -e /tmp/fhome/Documents/Screenshot.png ]' 'Screenshot.png stayed'

# 3. The terminal over B; a file dropped on it.
guest 'export XDG_RUNTIME_DIR=/tmp; /bin/terminal --token=t1 --timeout-s=500 --columns=60 --rows=20 > /tmp/t.log 2>&1 </dev/null & sleep 5; echo started' >/dev/null
set -- $(window 3)
tx=${1:-0}; ty=${2:-0}
drag $((tx + 60)) $((ty - 30)) $((tx + 60 + 300)) $((ty - 30))
tx=$((tx + 300))
drag $((ax + 440)) $((ay + 110)) $((tx + 200)) $((ty + 120))
expect_log /tmp/t.log 'ZTERM DROP enter uris=1 text=1'
expect_log /tmp/zdesktop.log "ZWL DATA drag accept client=$zc3 mime=text/uri-list"
expect_log /tmp/t.log 'ZTERM DROP bytes=[1-9][0-9]* uris=1'
expect_guest '[ -f /tmp/fhome/Desktop/Screenshot.png ]' 'Screenshot.png stayed (a copy)'
sleep 1
shot term-drop.png

errors=$(guest "grep -c ERROR /tmp/zdesktop.log" | tail -1)
[ "${errors:-1}" = 0 ] && echo "zdesktop: no ERROR" || { echo "zdesktop: ERROR lines"; status=1; }
guest 'grep -E "ZWL (DATA|MENU (open|context))" /tmp/zdesktop.log' > "$out/zdesktop-data.log"
guest 'cat /tmp/a.log' > "$out/a.log"
guest 'cat /tmp/b.log' > "$out/b.log"
guest 'cat /tmp/t.log' > "$out/t.log"
guest "$stop_all" >/dev/null
[ $status = 0 ] && echo "zdesktop-p088: PASS" || echo "zdesktop-p088: FAIL"
exit $status
