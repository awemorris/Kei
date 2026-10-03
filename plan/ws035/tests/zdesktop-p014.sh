#!/bin/sh
# ws035-p014: Wiseview (the window overview, p063) from the keyboard, on the Venus guest.
#
# zdesktop --glass runs at 1280x800 with three windows: a (wltest, pale, 420x300), s (wl_shm, dark
# 2b3444, 360x260) and b (wltest, pale blue, 380x280, on top).  Keys go through QMP.
#  1. Super+Tab opens Wiseview with the window on top (b) current: open.png.
#  2. Tab moves the current tile to s, Shift+Tab back to b, Right to s again: moved.png.
#  3. Enter chooses s: Wiseview closes and s is on top (selected.png shows its colour at its centre).
#  4. Super+Tab and Esc: Wiseview opens and closes.
#  5. Super+Tab twice: the second closes it.
#  6. Wiseview closed, a key reaches the windows again (Super+Tab is not taken while it closes).
#
#   plan/ws035/tests/zdesktop-guest.sh start     (the guest must be up)
#   plan/ws035/tests/zdesktop-p014.sh [OUTDIR]
# Copyright (C) 2026 Awe Morris; SPDX-License-Identifier: Zlib
set -u
cd "$(dirname -- "$0")/../../.."
GUEST_RUNTIME="${GUEST_RUNTIME:-$(pwd)/build/ws035-sq-run}"
export GUEST_RUNTIME
out=${1:-build/ws035-p014}
mkdir -p "$out"
guest() { timeout 90 python3 plan/tools/guest/guest.py run "$1" 2>&1; }
. plan/tools/guest/zwl-clients.sh
check() { python3 plan/ws035/tests/zdesktop-check.py "$@" --runtime "$GUEST_RUNTIME"; }
pointer() { python3 plan/ws035/tests/qmp-pointer.py "$GUEST_RUNTIME/qmp.sock" "$@"; }
keys() { python3 plan/ws035/tests/qmp-keys.py "$GUEST_RUNTIME/qmp.sock" "$@"; }
stop_all='service stop greeter >/dev/null 2>&1; for p in $(ps -A -o pid,args | grep -E "[w]ayland( |$)|[w]lshm|[w]ltest" | awk "{print \$1}"); do kill $p; done; i=0; while ps -A -o args | grep -qE "[w]ayland( |$)|[w]lshm|[w]ltest" && [ $i -lt 50 ]; do sleep 0.2; i=$((i+1)); done'
status=0

# Fails the run unless the compositor's log has (within a few seconds) as many lines matching a pattern as asked (default 1).
expect_log() {
	tries=0
	found=0
	while [ $tries -lt 5 ]; do
		found=$(guest "grep -cE '$1' /tmp/zdesktop.log" | tail -1)
		[ "${found:-0}" -ge "${2:-1}" ] 2>/dev/null && break
		tries=$((tries + 1))
		sleep 1
	done
	if [ "${found:-0}" -ge "${2:-1}" ] 2>/dev/null; then
		echo "log: $1 ok"
	else
		echo "log: $1 MISSING (found ${found:-0})"
		status=1
	fi
}

guest "$stop_all" >/dev/null
guest 'export XDG_RUNTIME_DIR=/tmp; rm -f /tmp/wayland-0; picture=; [ -f /usr/share/keiland/wallpaper.ppm ] && picture=--wallpaper=/usr/share/keiland/wallpaper.ppm
/bin/wayland --timeout=400 --width=1280 --height=800 --glass $picture > /tmp/zdesktop.log 2>&1 </dev/null & for w in 1 2 3 4 5 6 7 8 9 10 11 12 13 14 15 16 17 18 19 20 21 22 23 24 25 26 27 28 29 30; do grep -q ZWL.READY /tmp/zdesktop.log 2>/dev/null && break; sleep 0.5; done; sleep 1
/bin/wltest --windowed --size=420x300 --color=f4f7fc --frames=3000 --delay-ms=100 --token=a > /tmp/a.log 2>&1 </dev/null & sleep 2
/bin/wlshm --size=360x260 --color=ff2b3444 --frames=9000 --token=s > /tmp/s.log 2>&1 </dev/null & sleep 2
/bin/wltest --windowed --size=380x280 --color=dfe9f7 --frames=3000 --delay-ms=100 --token=b > /tmp/b.log 2>&1 </dev/null & sleep 4; echo started' >/dev/null
zwl_app_clients
s=$(guest "grep 'ZWL MAP client=$zc2 ' /tmp/zdesktop.log | tail -1" | sed -n 's/.* surface=\([0-9]*\) .*/\1/p')
b=$(guest "grep 'ZWL MAP client=$zc3 ' /tmp/zdesktop.log | tail -1" | sed -n 's/.* surface=\([0-9]*\) .*/\1/p')
echo "s is surface ${s:-?}, b is surface ${b:-?}"
pointer move 1250 780 sleep 300

# 1. Super+Tab opens Wiseview.
keys '<super-tab>'
sleep 1.5
check "$out/open.png" >/dev/null || status=1
expect_log 'WISEVIEW opening key'
expect_log 'WISEVIEW open windows=3'

# 2. Tab, Shift+Tab, Right: the current tile goes to s, back to b, and to s.
keys '<tab>'
expect_log "WISEVIEW current surface=$s\$"
keys '<shift-tab>'
expect_log "WISEVIEW current surface=$b\$"
keys '<right>'
sleep 0.5
check "$out/moved.png" >/dev/null || status=1
expect_log "WISEVIEW current surface=$s\$" 2

# 3. Enter chooses s: on top when Wiseview has closed.
keys '<ret>'
sleep 1.5
expect_log "WISEVIEW select surface=$s via=key"
expect_log 'WISEVIEW closed'
set -- $(guest "grep 'ZWL MAP client=$zc2 ' /tmp/zdesktop.log | tail -1" | sed -n 's/.* x=\([-0-9]*\) y=\([-0-9]*\).*/\1 \2/p')
check "$out/selected.png" --expect $(($1 + 180)),$(($2 + 130)),2b3444 || status=1

# 4. Super+Tab, then Esc.
keys '<super-tab>'
sleep 1
keys '<esc>'
sleep 1
expect_log 'WISEVIEW close key' 1
expect_log 'WISEVIEW closed' 2

# 5. Super+Tab twice.
keys '<super-tab>'
sleep 1
keys '<super-tab>'
sleep 1
expect_log 'WISEVIEW close key' 2
expect_log 'WISEVIEW closed' 3

# 6. Closed, it takes no more keys: Tab reaches the focused window, not Wiseview.
before=$(guest "grep -c 'WISEVIEW current' /tmp/zdesktop.log" | tail -1)
keys '<tab>'
sleep 0.5
after=$(guest "grep -c 'WISEVIEW current' /tmp/zdesktop.log" | tail -1)
echo "current lines before=$before after=$after"
[ "$before" = "$after" ] || status=1
check "$out/closed.png" --expect $(($1 + 180)),$(($2 + 130)),2b3444 || status=1

# Nothing failed.
guest 'grep -E "ERROR|FAILED" /tmp/zdesktop.log /tmp/a.log /tmp/s.log /tmp/b.log' | tee "$out/errors.txt"
[ -s "$out/errors.txt" ] && status=1
guest 'grep -E "WISEVIEW" /tmp/zdesktop.log' > "$out/log.txt"
guest "$stop_all" >/dev/null
[ $status -eq 0 ] && echo "p014: PASS" || echo "p014: FAIL"
exit $status
