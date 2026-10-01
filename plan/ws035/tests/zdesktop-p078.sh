#!/bin/sh
# ws035-p078: what zdesktop tells a toolkit about the keyboard and the output, on the Venus guest, with
# /bin/seat-probe (userland/tests/seat-probe): the XKB keymap (format xkb_v1, a keymap's text in the mapped
# file), the key repeat (25/s after 400 ms), the modifier masks (Shift held, Caps Lock and Num Lock locked), and
# wl_output version 4 (name, description, mode, scale, done; release at the end).  The keymap's content is checked on
# the host by plan/ws035/tests/p078/run-host.sh (libxkbcommon).
#
#   plan/tools/titlebar/menu-guest.sh start   (the lean image with /bin/seat-probe)
#   plan/ws035/tests/zdesktop-p078.sh [OUTDIR]
# Copyright (C) 2026 Awe Morris; SPDX-License-Identifier: Zlib
set -u
cd "$(dirname -- "$0")/../../.."
GUEST_RUNTIME="${GUEST_RUNTIME:-$(pwd)/build/ws035-sq-run}"
export GUEST_RUNTIME
out=${1:-build/ws035-p078}
mkdir -p "$out"
guest() { timeout 90 python3 plan/tools/guest/guest.py run "$1" 2>&1; }
check() { python3 plan/ws035/tests/zdesktop-check.py "$@" --runtime "$GUEST_RUNTIME"; }
keys() { python3 plan/ws035/tests/qmp-keys.py "$GUEST_RUNTIME/qmp.sock" "$@"; }
stop_all='service stop greeter >/dev/null 2>&1; for p in $(ps -A -o pid,args | grep -E "[w]ayland( |$)|[s]eat-probe" | awk "{print \$1}"); do kill $p; done; i=0; while ps -A -o args | grep -qE "[w]ayland( |$)|[s]eat-probe" && [ $i -lt 50 ]; do sleep 0.2; i=$((i+1)); done'
status=0

# Fails the run unless a log has (within a few seconds) as many lines matching a pattern as asked (default 1).
expect_log() {
	tries=0
	found=0
	while [ $tries -lt 5 ]; do
		found=$(guest "grep -cE '$2' $1" | tail -1)
		[ "${found:-0}" -ge "${3:-1}" ] 2>/dev/null && break
		tries=$((tries + 1))
		sleep 1
	done
	if [ "${found:-0}" -ge "${3:-1}" ] 2>/dev/null; then
		echo "log: $2 ok"
	else
		echo "log: $2 MISSING (found ${found:-0})"
		status=1
	fi
}

guest "$stop_all" >/dev/null
guest 'export XDG_RUNTIME_DIR=/tmp; rm -f /tmp/wayland-0; picture=; [ -f /usr/share/keiland/wallpaper.ppm ] && picture=--wallpaper=/usr/share/keiland/wallpaper.ppm
/bin/wayland --timeout=120 --width=1280 --height=800 --glass $picture > /tmp/zdesktop.log 2>&1 </dev/null & for w in 1 2 3 4 5 6 7 8 9 10 11 12 13 14 15 16 17 18 19 20 21 22 23 24 25 26 27 28 29 30; do grep -q ZWL.READY /tmp/zdesktop.log 2>/dev/null && break; sleep 0.5; done; sleep 1
/bin/seat-probe --timeout-s=40 --token=k > /tmp/k.log 2>&1 </dev/null & sleep 4; echo started' >/dev/null
expect_log /tmp/zdesktop.log 'ZWL KEYMAP format=xkb_v1 errno=0'
expect_log /tmp/k.log 'SEATPROBE ready run=k'

# The keymap, the repeat and the output.
expect_log /tmp/k.log 'SEATPROBE keymap format=1 size=[0-9]{4,} text=1 terminated=1'
expect_log /tmp/k.log 'SEATPROBE repeat rate=25 delay=400'
expect_log /tmp/k.log 'SEATPROBE output version=4'
expect_log /tmp/k.log 'SEATPROBE output geometry x=0 y=0 make=Unknown model=Unknown'
expect_log /tmp/k.log 'SEATPROBE output mode flags=3 width=1280 height=800 refresh=[0-9]+'
expect_log /tmp/k.log 'SEATPROBE output scale=1'
expect_log /tmp/k.log 'SEATPROBE output name=DISPLAY-1'
expect_log /tmp/k.log 'SEATPROBE output description=Display 1280x800'
expect_log /tmp/k.log 'SEATPROBE output done'
expect_log /tmp/k.log 'SEATPROBE focus'
check "$out/seat.png" >/dev/null

# Shift held: depressed 0x1 around the key; Caps Lock and Num Lock: locked 0x2, 0x12, then off again (Num Lock reaches
# the guest since BUG-070 was fixed; the keypad's 7 then comes as its own key, 71).
keys 'A'
expect_log /tmp/k.log 'SEATPROBE modifiers depressed=1 latched=0 locked=0 group=0'
expect_log /tmp/k.log 'SEATPROBE key 30 state=1'
keys '<caps_lock>'
expect_log /tmp/k.log 'SEATPROBE modifiers depressed=0 latched=0 locked=2 group=0'
keys '<num_lock>'
expect_log /tmp/k.log 'SEATPROBE modifiers depressed=0 latched=0 locked=18 group=0'
keys '<kp_7>'
expect_log /tmp/k.log 'SEATPROBE key 71 state=1'
keys '<caps_lock>' '<num_lock>'
expect_log /tmp/k.log 'SEATPROBE modifiers depressed=0 latched=0 locked=0 group=0' 2

# The probe ends by itself, releasing its output (a version 3 request), without an error.
sleep 30
expect_log /tmp/k.log 'SEATPROBE DONE run=k'
guest 'grep -E "ERROR|FAILED|protocol error" /tmp/zdesktop.log /tmp/k.log' | tee "$out/errors.txt"
[ -s "$out/errors.txt" ] && status=1
guest 'cat /tmp/k.log' > "$out/probe.log"
guest "$stop_all" >/dev/null
[ $status -eq 0 ] && echo "p078: PASS" || echo "p078: FAIL"
exit $status
