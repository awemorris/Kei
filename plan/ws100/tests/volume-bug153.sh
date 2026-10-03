#!/bin/sh
# ws100-p010 (BUG-153): a click on the system bar's slider is not undone by zdesktop's own earlier save read back.
# The volume image (build-volume-image.sh), kei's session at boot (volume-guest.sh starts the guest).
# Before the fix: zdesktop keeps a change in desktop.conf one second after it settles; its preferences watcher reads the
# file it wrote up to a second or two later and set audiod back to the kept volume, over a click made meanwhile (on the
# 5330: "50% を click しても次の瞬間に 100%").  The run:
#  1. The popup open; the slider clicked at its right end (100) and at its middle (50), five rounds, 1.3 s apart (each
#     click lands after the save of the click before, inside the window where the watcher reads that save back).
#  2. Five seconds later: audiod is at 49..51 (audiod-feedback get), zdesktop's last set is the middle click, and no
#     "ZWL VOLUME preferences value=" line came after the first click (the read-back is skipped: "skipped
#     reason=newer" or "reason=echo", or matches the kept values).  slider-50.png shows the knob in the middle.
#  3. Since BUG-161 (ws100-p012) nothing is written during the session and the file is not followed: a hand edit of
#     desktop.conf (sound.volume=30) leaves audiod as it is, and no "ZWL VOLUME kept" line comes before the session
#     ends.  Step 2 still holds (no read-back can undo a click, since there is no write to read back).
#   plan/ws100/tests/volume-bug153.sh IMAGE [OUTDIR]
# Copyright (C) 2026 Awe Morris; SPDX-License-Identifier: Zlib
set -u
cd "$(dirname -- "$0")/../../.."
image=${1:?usage: volume-bug153.sh IMAGE [OUTDIR]}
out=${2:-build/ws100-shots/bug153}
mkdir -p "$out"
GUEST_RUNTIME=$(pwd)/build/ws100-run
export GUEST_RUNTIME
log=/run/user/1000/session.log
conf=/home/kei/.config/keiland/desktop.conf
guest() { timeout 90 python3 plan/tools/guest/guest.py run "$1" 2>&1 </dev/null; }
pointer() { python3 plan/ws035/tests/qmp-pointer.py "$GUEST_RUNTIME/qmp.sock" "$@"; }
shot() { python3 plan/ws035/tests/zdesktop-check.py "$out/$1" --runtime "$GUEST_RUNTIME" >/dev/null 2>&1; echo "shot: $out/$1"; }
status=0

# Records a verdict.
verdict() {
	if [ "$1" = ok ]; then
		echo "$2 ok"
	else
		echo "$2 FAIL"
		status=1
	fi
}

# The number of lines of a guest file matching a pattern.
count() {
	guest "grep -cE '$2' $1" | tail -1
}

# Waits until a guest file has more than N lines matching a pattern (within some seconds); fails the run otherwise.
expect_more() {
	tries=0
	found=0
	while [ $tries -lt "$4" ]; do
		found=$(count "$1" "$2")
		[ "${found:-0}" -gt "$3" ] 2>/dev/null && break
		tries=$((tries + 1))
		sleep 1
	done
	if [ "${found:-0}" -gt "$3" ] 2>/dev/null; then
		echo "log: $2 ok"
		return 0
	fi
	echo "log: $2 MISSING"
	status=1
	return 1
}

# audiod's volume as "left muted".
audiod_volume() {
	guest 'audiod-feedback get' | sed -n 's/.*volume left=\([0-9]*\) right=[0-9]* muted=\([0-9]\).*/\1 \2/p' | tail -1
}

# The last line of the session log matching a pattern.
last() {
	guest "grep -E '$1' $log | tail -1"
}

# 0. The guest with HD Audio, and kei's session.
sh plan/ws100/tests/volume-guest.sh stop >/dev/null 2>&1
VOLUME_AUDIO=duplex timeout 180 sh plan/ws100/tests/volume-guest.sh start "$image" >/dev/null 2>&1
sleep 35
expect_more $log 'ZWL HANDOFF go=1' 0 60
expect_more $log 'ZWL VOLUME reachable=1 device=1' 0 10
set -- $(last 'ZWL VOLUME icon x=' | sed -n 's/.* x=\([0-9]*\) y=\([0-9]*\) width=\([0-9]*\) height=\([0-9]*\).*/\1 \2 \3 \4/p')
ix=$((${1:-900} + ${3:-30} / 2)); iy=$((${2:-3} + ${4:-28} / 2))

# 1. The popup, then the right end and the middle of the slider in turn.
pointer move $((ix - 2)) $iy sleep 200 move $ix $iy sleep 300 down sleep 60 up sleep 800
expect_more $log 'ZWL VOLUME popup open' 0 5
set -- $(last 'ZWL VOLUME popup open' | sed -n 's/.* x=\([0-9]*\) y=\([0-9]*\) width=\([0-9]*\) height=\([0-9]*\) slider=\([0-9]*\) mute=\([0-9]*\).*/\1 \2 \3 \4 \5 \6/p')
px=${1:-900} pw=${3:-260} slider=${5:-100}
track_left=$((px + 23)) track_width=$((pw - 46))
sy=$((slider + 17))
right=$((track_left + track_width + 4)) middle=$((track_left + track_width / 2))
first=$(guest "grep -c '' $log" | tail -1)
for round in 1 2 3 4 5; do
	pointer move $right $sy sleep 100 down sleep 60 up sleep 1300 move $middle $sy sleep 100 down sleep 60 up sleep 1300
done
sleep 5
shot slider-50.png

# 2. audiod and zdesktop at the middle; no read-back applied after the first click.
set -- $(audiod_volume)
[ "${1:-0}" -ge 49 ] && [ "${1:-0}" -le 51 ] && verdict ok "audiod at ${1:-?} after the last middle click" || verdict no "audiod at ${1:-?} after the last middle click"
lastset=$(last 'ZWL VOLUME set value=')
echo "last set: $lastset"
echo "$lastset" | grep -qE 'value=(49|50|51) .*final=1' && verdict ok "zdesktop's last set is the middle" || verdict no "zdesktop's last set is the middle"
guest "tail -n +$((${first:-0} + 1)) $log | grep -E 'ZWL VOLUME (set|saved|kept|preferences)'" > "$out/rounds.log"
cat "$out/rounds.log"
undone=$(grep -c 'ZWL VOLUME preferences value=' "$out/rounds.log")
[ "${undone:-1}" = 0 ] && verdict ok "no read-back applied over a click" || verdict no "read-back applied over a click ($undone)"

# 3. The file is neither written nor followed during the session (BUG-161): a hand edit leaves audiod as it is.
kept=$(grep -c 'ZWL VOLUME kept' "$out/rounds.log")
[ "${kept:-1}" = 0 ] && verdict ok "no write during the session" || verdict no "no write during the session ($kept)"
sleep 2
guest "grep -v '^sound\\.volume=' $conf > /tmp/bug153.conf; echo sound.volume=30 >> /tmp/bug153.conf; cat /tmp/bug153.conf > $conf; grep '^sound\\.volume' $conf" | tail -1
sleep 4
set -- $(audiod_volume)
[ "${1:-0}" -ge 49 ] && [ "${1:-0}" -le 51 ] && verdict ok "a hand edit is not followed (audiod at ${1:-?})" || verdict no "a hand edit is not followed (audiod at ${1:-?})"
errors=$(count $log 'ZWL ERROR')
[ "${errors:-1}" = 0 ] && verdict ok "no ZWL ERROR" || verdict no "ZWL ERROR ($errors)"
sh plan/ws100/tests/volume-guest.sh stop >/dev/null 2>&1
[ $status -eq 0 ] && echo "volume-bug153: PASS" || echo "volume-bug153: FAIL"
exit $status
