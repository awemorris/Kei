#!/bin/sh
# ws035-p079: the clipboard between clients (wl_data_device_manager v3) on the Venus guest, with /bin/data-probe
# (userland/tests/data-probe) and terminal.
#  1. Probe a (blue) sets its text as the selection (key s); probe b (green) is started on top: it gets the keyboard,
#     is told the selection (an offer with two text types) and receives a's text through a pipe.
#  2. a destroys its source (key q, after a press on a gives it the keyboard): the clipboard is empty; b is told so when
#     it gets the keyboard again.
#  3. b sets its text; a gets the keyboard and receives b's text; b's source is cancelled when a sets its own again.
#  4. The terminal: its Edit > Copy (ctrl+shift+a, ctrl+shift+c) sets the selection; probe c (started on top) receives it.
#     c sets "touch /tmp/p079-pasted" and a line break; the terminal's Edit > Paste (ctrl+shift+v) types it into the shell.
#
#   plan/tools/titlebar/menu-guest.sh start   (the lean image with /bin/data-probe)
#   plan/ws035/tests/zdesktop-p079.sh [OUTDIR]
# Copyright (C) 2026 Awe Morris; SPDX-License-Identifier: Zlib
set -u
cd "$(dirname -- "$0")/../../.."
GUEST_RUNTIME="${GUEST_RUNTIME:-$(pwd)/build/ws035-sq-run}"
export GUEST_RUNTIME
out=${1:-build/ws035-p079}
mkdir -p "$out"
guest() { timeout 90 python3 plan/tools/guest/guest.py run "$1" 2>&1; }
. plan/tools/guest/zwl-clients.sh
check() { python3 plan/ws035/tests/zdesktop-check.py "$@" --runtime "$GUEST_RUNTIME"; }
pointer() { python3 plan/ws035/tests/qmp-pointer.py "$GUEST_RUNTIME/qmp.sock" "$@"; }
keys() { python3 plan/ws035/tests/qmp-keys.py "$GUEST_RUNTIME/qmp.sock" "$@"; }
stop_all='service stop greeter >/dev/null 2>&1; for p in $(ps -A -o pid,args | grep -E "[w]ayland( |$)|[d]ata-probe|[t]erminal" | awk "{print \$1}"); do kill $p; done; i=0; while ps -A -o args | grep -qE "[w]ayland( |$)|[d]ata-probe|[t]erminal" && [ $i -lt 50 ]; do sleep 0.2; i=$((i+1)); done'
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

# The place of a client's window (from zdesktop's MAP line).
window_of() {
	guest "grep 'ZWL MAP client=$(zwl_app_client $1) ' /tmp/zdesktop.log | tail -1" | sed -n 's/.* x=\([-0-9]*\) y=\([-0-9]*\).*/\1 \2/p'
}

# Clicks a point (with a small approach, so that the motion is seen before the press).
click() {
	pointer move $(($1 - 2)) "$2" sleep 150 move "$1" "$2" sleep 300 down sleep 60 up sleep "${3:-800}"
}

guest "$stop_all" >/dev/null
guest 'rm -f /tmp/p079-pasted; export XDG_RUNTIME_DIR=/tmp; rm -f /tmp/wayland-0; picture=; [ -f /usr/share/keiland/wallpaper.ppm ] && picture=--wallpaper=/usr/share/keiland/wallpaper.ppm
/bin/wayland --timeout=400 --width=1280 --height=800 --glass $picture > /tmp/zdesktop.log 2>&1 </dev/null & for w in 1 2 3 4 5 6 7 8 9 10 11 12 13 14 15 16 17 18 19 20 21 22 23 24 25 26 27 28 29 30; do grep -q ZWL.READY /tmp/zdesktop.log 2>/dev/null && break; sleep 0.5; done; sleep 1
/bin/data-probe --token=a --color=3060c0 --text="hello from a" --timeout-s=300 > /tmp/a.log 2>&1 </dev/null & sleep 3; echo started' >/dev/null
expect_log /tmp/a.log 'DATAPROBE ready run=a'
expect_log /tmp/a.log 'DATAPROBE focus'

# 1. a sets the selection; b starts on top and receives a's text.
keys 's'
expect_log /tmp/a.log 'DATAPROBE set selection text=hello from a'
zwl_app_clients
expect_log /tmp/zdesktop.log "ZWL DATA selection client=$zc1 source=[0-9]+ types=2"
guest 'export XDG_RUNTIME_DIR=/tmp; /bin/data-probe --token=b --color=30a050 --text="b says hi" --timeout-s=300 > /tmp/b.log 2>&1 </dev/null & sleep 3; echo started' >/dev/null
expect_log /tmp/b.log 'DATAPROBE ready run=b'
expect_log /tmp/b.log 'DATAPROBE type text/plain;charset=utf-8'
expect_log /tmp/b.log 'DATAPROBE selection text=1'
expect_log /tmp/a.log 'DATAPROBE send mime=text/plain;charset=utf-8 bytes=12'
expect_log /tmp/b.log 'DATAPROBE received bytes=12 text=hello from a'
check "$out/two.png" >/dev/null

# 2. a (pressed, so it has the keyboard) destroys its source; b hears the empty clipboard when it has the keyboard again.
set -- $(window_of 1); ax=${1:-0}; ay=${2:-0}
set -- $(window_of 2); bx=${1:-0}; by=${2:-0}
echo "a at $ax,$ay b at $bx,$by"
click $((ax + 20)) $((ay + 20))
expect_log /tmp/a.log 'DATAPROBE focus' 2
keys 'q'
expect_log /tmp/a.log 'DATAPROBE source destroyed'
expect_log /tmp/zdesktop.log 'ZWL DATA selection none \(source gone\)'
click $((bx + 285)) $((by + 190))
expect_log /tmp/b.log 'DATAPROBE selection none'

# 3. b sets its text; a receives it when it has the keyboard; a's own selection then cancels b's source.
keys 's'
expect_log /tmp/b.log 'DATAPROBE set selection text=b says hi'
click $((ax + 20)) $((ay + 20))
expect_log /tmp/a.log 'DATAPROBE received bytes=9 text=b says hi'
keys 's'
expect_log /tmp/b.log 'DATAPROBE cancelled'

# 4. The terminal: Copy sets the selection, c receives the terminal's text; Paste types c's text into the shell.
guest 'export XDG_RUNTIME_DIR=/tmp; HOME=/root /bin/terminal > /tmp/t.log 2>&1 </dev/null & sleep 4; echo started' >/dev/null
sleep 2
keys 'echo p079-copy-me' '<ret>'
sleep 1
keys '<ctrl-shift-a>' '<ctrl-shift-c>'
expect_log /tmp/zdesktop.log "ZWL DATA selection client=$zc3 "
check "$out/terminal-copy.png" >/dev/null
guest 'export XDG_RUNTIME_DIR=/tmp; /bin/data-probe --token=c --color=a05030 --text="touch /tmp/p079-pasted
" --timeout-s=300 > /tmp/c.log 2>&1 </dev/null & sleep 3; echo started' >/dev/null
expect_log /tmp/c.log 'DATAPROBE focus'
expect_log /tmp/c.log 'DATAPROBE received bytes=[0-9]+ text=.*p079-copy-me'
guest 'grep -A3 "received" /tmp/c.log' > "$out/terminal-text.txt"
keys 's'
expect_log /tmp/c.log 'DATAPROBE set selection'
set -- $(window_of 3); tx=${1:-0}; ty=${2:-0}
click $((tx + 600)) $((ty + 400))
keys '<ctrl-shift-v>'
expect_log /tmp/c.log 'DATAPROBE send mime=text/plain;charset=utf-8 bytes=23'
sleep 2
pasted=$(guest 'ls /tmp/p079-pasted 2>/dev/null' | tail -1)
[ "$pasted" = /tmp/p079-pasted ] && echo "terminal paste ran the command: ok" || { echo "terminal paste ran the command: MISSING"; status=1; }
check "$out/terminal-paste.png" >/dev/null

# Nothing failed.
guest 'grep -E "ERROR|FAILED|protocol error" /tmp/zdesktop.log /tmp/a.log /tmp/b.log /tmp/c.log' | tee "$out/errors.txt"
[ -s "$out/errors.txt" ] && status=1
guest 'cat /tmp/a.log; cat /tmp/b.log; cat /tmp/c.log' > "$out/probes.log"
guest 'grep -E "DATA|CLIENT|MAP" /tmp/zdesktop.log' > "$out/zdesktop.log"
guest 'grep -E "ZTERM (COPY|PASTE|CLIPBOARD)" /tmp/t.log' > "$out/terminal.log"
guest "$stop_all" >/dev/null
[ $status -eq 0 ] && echo "p079: PASS" || echo "p079: FAIL"
exit $status
