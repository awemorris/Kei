#!/bin/sh
# ws035-p128: a floating window's frame resizes it from its four corners (and its sides), on the Venus guest of the
# graphical login image (plan/ws035/tests/build-login-image.sh BUILD graphical; kei is logged in at boot).
# Files is opened from App Home (ZWL GLASS launch ... to=X,Y size=WxH; launch-late when its window came after
# the 5 s the grow waits for, ws099-p024 BUG-147); then, for each corner in turn, the pointer
#  1. rests on the frame just outside the corner: the cursor becomes the corner's diagonal arrow (ZWL CURSOR frame
#     edges=E: 5 top-left, 9 top-right, 6 bottom-left, 10 bottom-right), photographed as OUTDIR/hover-CORNER.png, and
#  2. drags the corner STEP pixels outwards: the resize starts with both sides (ZWL RESIZE start edges=E) and, once
#     settled (ZWL RESIZE settled x= y= width= height=), the window is STEP wider and higher and the corner's
#     opposite corner has not moved.
# Last the right side is dragged the same way (edges=8, the width grows, the height stays).  Screens: OUTDIR/*.png.
#
# The output is the demonstration's 1920x1280 (VENUS_SIZE, the same for the guest and this test), where Files opens
# with room around it; the frame's arrow and press give way to the screen's strips (the system bar, the desktops'
# swipe at the sides, Wiseview's at the bottom).
#
#   VENUS_SIZE=1920x1280 GUEST_RUNTIME=build/ws035-run plan/ws035/tests/zdesktop-guest.sh start build/amd64/hdd-image.img
#   VENUS_SIZE=1920x1280 GUEST_RUNTIME=build/ws035-run plan/ws035/tests/zdesktop-p128.sh [OUTDIR]
# Copyright (C) 2026 Awe Morris; SPDX-License-Identifier: Zlib
set -u
cd "$(dirname -- "$0")/../../.."
GUEST_RUNTIME="${GUEST_RUNTIME:-$(pwd)/build/ws035-run}"
export GUEST_RUNTIME
out=${1:-build/ws035-p128}
step=${STEP:-40}
size=${VENUS_SIZE:-1920x1280}
mkdir -p "$out"
log=/run/user/1000/session.log
# The SSH to the guest, tried again when ssh itself fails (plan/ws099/tests/guest-retry.sh, ws099-p023).
. plan/ws099/tests/guest-retry.sh
guest() { guest_retry 120 "$1" </dev/null; }
pointer() { python3 plan/ws035/tests/qmp-pointer.py --width "${size%x*}" --height "${size#*x}" "$GUEST_RUNTIME/qmp.sock" "$@"; }
shot() { python3 plan/ws035/tests/zdesktop-check.py "$out/$1.png" --runtime "$GUEST_RUNTIME" >/dev/null 2>&1; }
status=0

# The number of lines of the session's log matching a pattern.
count() {
	guest "grep -cE '$1' $log" | tail -1
}

# Waits until the session's log has more than BEFORE lines matching a pattern (within some seconds).
expect_more() {
	tries=0
	found=0
	while [ $tries -lt "$3" ]; do
		found=$(count "$1")
		[ "${found:-0}" -gt "$2" ] 2>/dev/null && break
		tries=$((tries + 1))
		sleep 1
	done
	if [ "${found:-0}" -gt "$2" ] 2>/dev/null; then
		echo "log: $1 ok"
		return 0
	fi
	echo "log: $1 MISSING"
	status=1
	return 1
}

# Reads the window's outline (the body; its title bar is 52 pixels above it) from the last settled resize, or the launch.
geometry() {
	line=$(guest "grep -E 'ZWL RESIZE settled surface=$surface ' $log | tail -1")
	if [ -n "$line" ]; then
		set -- $(echo "$line" | sed -n 's/.* x=\(-*[0-9]*\) y=\(-*[0-9]*\) width=\([0-9]*\) height=\([0-9]*\).*/\1 \2 \3 \4/p')
	else
		set -- $(guest "grep -E 'ZWL GLASS launch(-late)? surface=$surface ' $log | tail -1" | sed -n 's/.* to=\(-*[0-9]*\),\(-*[0-9]*\) size=\([0-9]*\)x\([0-9]*\).*/\1 \2 \3 \4/p')
	fi
	x=$1 y=$2 width=$3 height=$4
	left=$x right=$((x + width)) top=$((y - 52)) bottom=$((y + height))
}

# Rests on one frame point, then drags it by (DX, DY); checks the cursor, the resize's edges and the new outline.
drag() {
	name=$1 edges=$2 px=$3 py=$4 dx=$5 dy=$6 grow_w=$7 grow_h=$8
	hovers=$(count "ZWL CURSOR frame edges=$edges\$")
	starts=$(count "ZWL RESIZE start surface=$surface edges=$edges ")
	settles=$(count "ZWL RESIZE settled surface=$surface ")
	old_left=$left old_right=$right old_top=$top old_bottom=$bottom old_width=$width old_height=$height
	pointer move $((px - 6)) $((py - 6)) sleep 150 move "$px" "$py" sleep 300
	expect_more "ZWL CURSOR frame edges=$edges\$" "$hovers" 5
	shot hover-$name
	pointer down sleep 100 move $((px + dx / 2)) $((py + dy / 2)) sleep 100 move $((px + dx)) $((py + dy)) sleep 300 up sleep 200
	expect_more "ZWL RESIZE start surface=$surface edges=$edges " "$starts" 5
	expect_more "ZWL RESIZE settled surface=$surface " "$settles" 10 || return
	geometry
	echo "$name: $old_width x $old_height at $old_left,$old_top -> $width x $height at $left,$top"
	if [ $((width - old_width)) -ne "$grow_w" ] || [ $((height - old_height)) -ne "$grow_h" ]; then
		echo "$name: size changed by $((width - old_width)) x $((height - old_height)), not $grow_w x $grow_h FAIL"
		status=1
	fi
	# The corner (or side) opposite the dragged one stays.
	case $edges in
	5) [ "$right" -eq "$old_right" ] && [ "$bottom" -eq "$old_bottom" ] || { echo "$name: the bottom-right corner moved FAIL"; status=1; } ;;
	9) [ "$left" -eq "$old_left" ] && [ "$bottom" -eq "$old_bottom" ] || { echo "$name: the bottom-left corner moved FAIL"; status=1; } ;;
	6) [ "$right" -eq "$old_right" ] && [ "$top" -eq "$old_top" ] || { echo "$name: the top-right corner moved FAIL"; status=1; } ;;
	10|8) [ "$left" -eq "$old_left" ] && [ "$top" -eq "$old_top" ] || { echo "$name: the top-left corner moved FAIL"; status=1; } ;;
	esac
	shot after-$name
}

# kei's session, then Files from App Home.
expect_more 'ZWL HANDOFF go=1' 0 90
sleep 3
launches=$(count 'ZWL GLASS launch(-late)? surface=')
pointer move 23 17 sleep 300 down sleep 60 up sleep 1500
set -- $(guest "grep 'ZWL HOME icon name=\"Files\"' $log | tail -1" | sed -n 's/.* x=\([0-9]*\) y=\([0-9]*\).*/\1 \2/p')
if [ -z "${1:-}" ]; then
	echo "no Files icon"
	exit 1
fi
pointer move "$1" "$2" sleep 200 down sleep 60 up
expect_more 'ZWL GLASS launch(-late)? surface=' "$launches" 20 || exit 1
sleep 3
surface=$(guest "grep -E 'ZWL GLASS launch(-late)? surface=' $log | tail -1" | sed -n 's/.*surface=\([0-9]*\) .*/\1/p')
geometry
echo "Files: surface $surface, $width x $height at $left,$top (title bar top) to $right,$bottom"
shot opened

# The corners, each dragged outwards; the window grows by STEP each way every time.
geometry
drag bottom-right 10 $((right + 3)) $((bottom + 3)) "$step" "$step" "$step" "$step"
geometry
drag top-left 5 $((left - 4)) $((top - 4)) $((-step)) $((-step)) "$step" "$step"
geometry
drag top-right 9 $((right + 3)) $((top - 4)) "$step" $((-step)) "$step" "$step"
geometry
drag bottom-left 6 $((left - 4)) $((bottom + 3)) $((-step)) "$step" "$step" "$step"

# A side still resizes by that side only.
geometry
drag right 8 $((right + 3)) $(((top + bottom) / 2)) "$step" 0 "$step" 0

echo "zdesktop-p128: status=$status"
exit $status
