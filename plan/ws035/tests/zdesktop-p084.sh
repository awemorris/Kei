#!/bin/sh
# ws035-p084: drag and drop between windows (wl_data_device) on the Venus guest (the lean image,
# plan/tools/files/build-files-image.sh).  zdesktop --glass at 1280x800; two files windows of
# 600x560 side by side on the sample home: A on Desktop (left), B on Documents (right).
#  1. two.png: the windows moved apart by their titlebars.
#  2. Logo.png (A's first item) dragged out of A over B's content: A starts the drag (DRAG out, DND start),
#     B hears enter and accepts the file names; over.png is taken with the button held (zdesktop's badge);
#     the release drops it on B, which reads the names and moves the file into Documents (DROP operation=move),
#     finishes, and A hears its drag is over (DRAG out done dropped=1).
#  3. Logo.png, now B's, dragged onto the first part (Home) of A's titlebar path: A's titlebar hears the part
#     (drop_target id=4 detail=0), the drop moves the file to the home folder; crumb.png.
#  4. Screenshot.png dragged out of A and onto A's own path (Home): the window's own drag (self=1) moves it home.
#  5. A drag out of A released over the wallpaper: cancelled (drag cancel, DRAG out done dropped=0); the file
#     stays.
#
#   plan/tools/files/files-guest.sh start     (the guest must be up)
#   plan/ws035/tests/zdesktop-p084.sh [OUTDIR]
# Copyright (C) 2026 Awe Morris; SPDX-License-Identifier: Zlib
set -u
cd "$(dirname -- "$0")/../../.."
GUEST_RUNTIME="${GUEST_RUNTIME:-$(pwd)/build/ws071-run}"
export GUEST_RUNTIME
out=${1:-build/ws035-p084}
mkdir -p "$out"
guest() { timeout 90 python3 plan/tools/guest/guest.py run "$1" 2>&1; }
. plan/tools/guest/zwl-clients.sh
check() { python3 plan/ws035/tests/zdesktop-check.py "$@" --runtime "$GUEST_RUNTIME"; }
pointer() { python3 plan/ws035/tests/qmp-pointer.py "$GUEST_RUNTIME/qmp.sock" "$@"; }
stop_all='service stop greeter >/dev/null 2>&1; for p in $(ps -A -o pid,args | grep -E "[w]ayland( |$)|[f]iles" | awk "{print \$1}"); do kill $p; done; i=0; while ps -A -o args | grep -qE "[w]ayland( |$)|[f]iles" && [ $i -lt 50 ]; do sleep 0.2; i=$((i+1)); done'
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

# A window's place from zdesktop's log: "x y" of client N's map.
window() {
	guest "grep 'ZWL MAP client=$(zwl_app_client $1) ' /tmp/zdesktop.log | tail -1" | sed -n 's/.* x=\([-0-9]*\) y=\([-0-9]*\).*/\1 \2/p'
}

# A titlebar control's rectangle from zdesktop's log: "x y width height".
place() {
	guest "grep 'ZWL TITLEBAR control client=$(zwl_app_client $1) .* where=floating id=$2 ' /tmp/zdesktop.log | tail -1" |
	    sed -n 's/.* x=\([-0-9]*\) y=\([-0-9]*\) width=\([0-9]*\) height=\([0-9]*\).*/\1 \2 \3 \4/p'
}

# Presses a screen point, moves with the button held to another in steps, and stays down.
drag() {
	pointer move $(($1 - 2)) "$2" sleep 150 move "$1" "$2" sleep 300 down sleep 120 \
	    move $(($1 + 12)) $(($2 + 4)) sleep 100 \
	    move $((($1 + $3) / 2)) $((($2 + $4) / 2)) sleep 150 \
	    move $(($3 + 3)) $(($4 + 1)) sleep 150 \
	    move "$3" "$4" sleep 700
}
release() {
	pointer up sleep 1500
}
shot() {
	pointer move 1270 790 sleep 400
	check "$out/$1" >/dev/null
}

guest "$stop_all" >/dev/null
guest 'rm -f /tmp/files.clipboard; rm -rf /tmp/fhome; sh /usr/share/files-tests/make-home.sh /tmp/fhome >/dev/null'
guest 'export XDG_RUNTIME_DIR=/tmp; rm -f /tmp/wayland-0
/bin/wayland --timeout=900 --width=1280 --height=800 --glass --wallpaper=/usr/share/keiland/wallpaper.ppm > /tmp/zdesktop.log 2>&1 </dev/null & for w in 1 2 3 4 5 6 7 8 9 10 11 12 13 14 15 16 17 18 19 20 21 22 23 24 25 26 27 28 29 30; do grep -q ZWL.READY /tmp/zdesktop.log 2>/dev/null && break; sleep 0.5; done; sleep 1
HOME=/tmp/fhome /bin/files --token=a --timeout-s=800 --width=600 --height=560 /tmp/fhome/Desktop > /tmp/a.log 2>&1 </dev/null & sleep 6; echo started' >/dev/null

# 1. A to the left by its title, then B, to the right.
set -- $(window 1)
ax=${1:-0}; ay=${2:-0}
ax0=$ax
drag $((ax + 60)) $((ay - 30)) $((ax + 60 - 320)) $((ay - 30))
release
guest 'export XDG_RUNTIME_DIR=/tmp; HOME=/tmp/fhome /bin/files --token=b --timeout-s=800 --width=600 --height=560 /tmp/fhome/Documents > /tmp/b.log 2>&1 </dev/null & sleep 6; echo started' >/dev/null
set -- $(window 2)
bx=${1:-0}; by=${2:-0}
drag $((bx + 60)) $((by - 30)) $((bx + 60 + 288)) $((by - 30))
release
ax=$((ax - 320)); bx=$((bx + 288))
echo "A at $ax,$ay  B at $bx,$by"
expect_log /tmp/a.log 'ZFILES LOCATION kind=folder path=/tmp/fhome/Desktop items=2 error=0'
expect_log /tmp/b.log 'ZFILES LOCATION kind=folder path=/tmp/fhome/Documents items=6 error=0'
shot two.png

# 2. Logo.png from A onto B's content.
drag $((ax + 330)) $((ay + 110)) $((bx + 420)) $((by + 420))
expect_log /tmp/a.log 'ZFILES DRAG out items=1$'
zwl_app_clients
expect_log /tmp/zdesktop.log "ZWL DATA drag start client=$zc1 source=[0-9]+ types=2 actions=7 "
expect_log /tmp/zdesktop.log "ZWL DATA drag enter client=$zc2 "
expect_log /tmp/zdesktop.log "ZWL DATA drag accept client=$zc2 mime=text/uri-list"
check "$out/over.png" >/dev/null
release
expect_log /tmp/zdesktop.log "ZWL DATA drag drop client=$zc1 target=$zc2 "
expect_log /tmp/b.log 'ZFILES DROP operation=move items=1 destination=/tmp/fhome/Documents first=/tmp/fhome/Desktop/Logo.png'
expect_log /tmp/zdesktop.log "ZWL DATA drag finish client=$zc2 "
expect_log /tmp/a.log 'ZFILES DRAG out done dropped=1'
expect_guest '[ -f /tmp/fhome/Documents/Logo.png ] && [ ! -e /tmp/fhome/Desktop/Logo.png ]' 'Logo.png moved to Documents'
shot moved.png

# 3. Logo.png (the third of B's items by name) onto A's first path part (logged where A was mapped; A moved since).
sleep 2
set -- $(place 1 4)
crumb_x=$((${1:-0} - ax0 + ax + 10)); crumb_y=$((${2:-0} + ${4:-0} / 2))
logo=$(guest "ls /tmp/fhome/Documents | sort -f | grep -n Logo.png | cut -d: -f1" | tail -1)
echo "crumb at $crumb_x,$crumb_y; Logo.png is item ${logo:-?} in B"
drag $((bx + 330 + ((${logo:-1} - 1) % 3) * 110)) $((by + 110 + ((${logo:-1} - 1) / 3) * 120)) "$crumb_x" "$crumb_y"
expect_log /tmp/zdesktop.log "ZWL TITLEBAR drop_target client=$zc1 id=4 detail=0"
check "$out/crumb.png" >/dev/null
release
expect_log /tmp/a.log 'ZFILES DROP operation=move items=1 destination=/tmp/fhome first=/tmp/fhome/Documents/Logo.png'
expect_guest '[ -f /tmp/fhome/Logo.png ] && [ ! -e /tmp/fhome/Documents/Logo.png ]' 'Logo.png moved home by the path'

# 4. Screenshot.png (A's only item now) onto A's own path.
sleep 2
drag $((ax + 330)) $((ay + 110)) "$crumb_x" "$crumb_y"
expect_log /tmp/a.log 'ZFILES DROP enter self=1 '
release
expect_log /tmp/a.log 'ZFILES DROP drop self=1 destination=/tmp/fhome '
expect_guest '[ -f /tmp/fhome/Screenshot.png ] && [ ! -e /tmp/fhome/Desktop/Screenshot.png ]' 'Screenshot.png moved home by its own path'
shot self.png

# 5. A drag from B released over the wallpaper: nothing moves.
drag $((bx + 330)) $((by + 110)) 640 760
release
expect_log /tmp/zdesktop.log "ZWL DATA drag cancel client=$zc2 reason=release"
expect_log /tmp/b.log 'ZFILES DRAG out done dropped=0'
count=$(guest "ls /tmp/fhome/Documents | wc -l" | tail -1)
[ "${count:-0}" -eq 6 ] 2>/dev/null && echo "cancel: Documents keeps 6 ok" || { echo "cancel: Documents has ${count:-?} MISSING"; status=1; }

errors=$(guest "grep -c ERROR /tmp/zdesktop.log" | tail -1)
[ "${errors:-1}" = 0 ] && echo "zdesktop: no ERROR" || { echo "zdesktop: ERROR lines"; status=1; }
guest 'grep -E "ZWL (DATA|TITLEBAR drop)" /tmp/zdesktop.log' > "$out/zdesktop-data.log"
guest 'cat /tmp/a.log' > "$out/a.log"
guest 'cat /tmp/b.log' > "$out/b.log"
guest "$stop_all" >/dev/null
[ $status = 0 ] && echo "zdesktop-p084: PASS" || echo "zdesktop-p084: FAIL"
exit $status
