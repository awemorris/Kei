#!/bin/sh
# ws095-p004: the input method's protocols and key path on the Venus guest (the lean image, build-ime-image.sh).
# zdesktop --glass at 1280x800 starts /usr/libexec/keiland-ime; the test client ime-probe logs what its text input
# hears (/tmp/p.log, read over SSH).  Checks:
#  1. The input method starts and is ready (ZWL IME started, KEI-IME READY); the probe does not see its globals.
#  2. Direct input: keys reach the probe as keys (PROBE KEY), the input method is activated for it.
#  3. Alt+Space chooses Japanese (ZWL IME language=ja); "kanji" is a preedit, Space converts it, Enter commits 漢字;
#     "watasi" Space Enter commits 私; every done's serial is the number of the probe's commits.
#  4. Alt+Space goes back to direct input; x reaches the probe as a key.
#  5. A password field (ime-probe --password): the input method is not activated and a is a key, not a preedit.
#  6. The input method stopped (SIGSTOP): a key is passed by after 500 ms (ZWL IME bypass) and the next reaches the
#     probe; SIGCONT: it answers again (ZWL IME answering).
#  7. The input method killed: zdesktop lets go (ZWL IME lost) and starts it again (a second KEI-IME READY).
#
#   plan/ws095/tests/ime-guest.sh start     (the guest must be up)
#   plan/ws095/tests/ime-p004.sh [OUTDIR]   (default build/ws095-shots/p004)
# Copyright (C) 2026 Awe Morris; SPDX-License-Identifier: Zlib
set -u
cd "$(dirname -- "$0")/../../.."
GUEST_RUNTIME="${GUEST_RUNTIME:-$(pwd)/build/ws095-run}"
export GUEST_RUNTIME
out=${1:-build/ws095-shots/p004}
mkdir -p "$out"
guest() { timeout 90 python3 plan/tools/guest/guest.py run "$1" 2>&1 </dev/null; }
check() { python3 plan/ws035/tests/zdesktop-check.py "$@" --runtime "$GUEST_RUNTIME"; }
keys() { python3 plan/ws035/tests/qmp-keys.py "$GUEST_RUNTIME/qmp.sock" "$@"; sleep 0.7; }
stop_all='service stop greeter >/dev/null 2>&1; for p in $(ps -A -o pid,args | grep -E "[w]ayland( |$)|[i]me-probe|[k]eiland-ime" | awk "{print \$1}"); do kill -CONT $p 2>/dev/null; kill $p; done; i=0; while ps -A -o args | grep -qE "[w]ayland( |$)|[i]me-probe|[k]eiland-ime" && [ $i -lt 50 ]; do sleep 0.2; i=$((i+1)); done'
start_desktop='export XDG_RUNTIME_DIR=/tmp HOME=/root; rm -f /tmp/wayland-0 /root/.config/kei/ime/ja-user.dict; picture=; [ -f /usr/share/keiland/wallpaper.ppm ] && picture=--wallpaper=/usr/share/keiland/wallpaper.ppm
/bin/wayland --timeout=900 --width=1280 --height=800 --glass $picture > /tmp/zdesktop.log 2>&1 </dev/null & sleep 4; echo started'
status=0

# Counts the lines of a guest file that match a pattern (read over SSH and matched on the host, whose grep knows
# UTF-8; a Japanese pattern does not survive the guest's shell).
count() {
	guest "cat $1" > "$out/.count" 2>/dev/null
	n=$(LC_ALL=C.UTF-8 grep -cE "$2" "$out/.count")
	case "$n" in ''|*[!0-9]*) n=0;; esac
	echo "$n"
}

# Fails the run unless a log has at least N lines matching a pattern (within a few seconds).
expect_log() {
	want=${3:-1}
	tries=0
	found=0
	while [ $tries -lt 10 ]; do
		found=$(count "$1" "$2")
		[ "$found" -ge "$want" ] && break
		tries=$((tries + 1))
		sleep 1
	done
	if [ "$found" -ge "$want" ]; then
		echo "log: $2 ok ($found)"
	else
		echo "log: $2 MISSING ($found of $want)"
		status=1
	fi
}

# Fails the run if a log has a line matching a pattern.
expect_none() {
	found=$(count "$1" "$2")
	if [ "$found" -eq 0 ]; then
		echo "log: no $2 ok"
	else
		echo "log: $2 UNEXPECTED ($found)"
		status=1
	fi
}

# A picture of the screen.
shot() {
	check "$out/$1" >/dev/null
	echo "shot: $out/$1"
}

# Starts a probe (its log at $1), with more options after.
start_probe() {
	log=$1; shift
	guest "export XDG_RUNTIME_DIR=/tmp HOME=/root; /bin/ime-probe --log=$log $* > /dev/null 2>&1 </dev/null & echo \$! > $log.pid; sleep 3; echo started" >/dev/null
	expect_log "$log" 'PROBE READY'
	expect_log "$log" 'PROBE ENTER'
}

# 1. The input method starts; the probe does not see its globals.
guest "$stop_all" >/dev/null
guest "$start_desktop" >/dev/null
expect_log /tmp/zdesktop.log 'ZWL IME started pid='
expect_log /tmp/zdesktop.log 'KEI-IME READY languages=2'
expect_log /tmp/zdesktop.log 'ZWL IME language=direct'
start_probe /tmp/p.log
expect_none /tmp/p.log 'PROBE SEES'
expect_log /tmp/zdesktop.log 'ZWL IME activate client='

# 2. Direct input: the keys are the probe's.
keys 'ab'
expect_log /tmp/p.log 'PROBE KEY key=30 state=1'
expect_log /tmp/p.log 'PROBE KEY key=48 state=1'
expect_none /tmp/p.log 'preedit=[^ ]'

# 3. Japanese: a preedit, a conversion, a commit.
keys '<alt-spc>'
expect_log /tmp/zdesktop.log 'ZWL IME language=ja'
expect_log /tmp/zdesktop.log 'KEI-IME LANGUAGE id=ja'
keys 'kanji'
expect_log /tmp/p.log 'preedit=かんじ '
shot preedit.png
keys ' '
expect_log /tmp/p.log 'preedit=漢字 '
shot converted.png
keys '\n'
expect_log /tmp/p.log 'PROBE TEXT text=漢字$'
keys 'watasi' ' ' '\n'
expect_log /tmp/p.log 'PROBE TEXT text=漢字私$'
bad=$(guest "awk '/PROBE DONE/ { split(\$3, s, \"=\"); split(\$4, c, \"=\"); if (s[2] != c[2]) bad++ } END { print bad + 0 }' /tmp/p.log" | tail -1)
dones=$(count /tmp/p.log 'PROBE DONE')
if [ "$bad" = 0 ] && [ "$dones" -gt 0 ]; then
	echo "serials: $dones dones, each the number of the probe's commits ok"
else
	echo "serials: $bad of $dones dones name another number FAIL"
	status=1
fi
expect_none /tmp/p.log 'PROBE KEY key=37 state=1'

# 4. Back to direct input.
keys '<alt-spc>'
expect_log /tmp/zdesktop.log 'ZWL IME language=direct' 2
keys 'x'
expect_log /tmp/p.log 'PROBE KEY key=45 state=1'

# 5. A password field is not served: Japanese chosen, a is a key.
keys '<alt-spc>'
expect_log /tmp/zdesktop.log 'ZWL IME language=ja' 2
start_probe /tmp/pw.log --password
keys 'a'
expect_log /tmp/pw.log 'PROBE KEY key=30 state=1'
expect_none /tmp/pw.log 'preedit=[^ ]'
shot password.png
# Only the password field's probe goes (ws095-p005: by the pid it started with; the guest's ps shows no arguments,
# and killing every ime-probe closed the first window too, which was taken for BUG-113).
guest 'kill $(cat /tmp/pw.log.pid); sleep 2' >/dev/null

# A new probe for the rest (zdesktop gives the keyboard to the next window only when one is shown).
start_probe /tmp/p2.log
expect_log /tmp/zdesktop.log 'ZWL IME activate client=' 2

# 6. A stopped input method is passed by, and heard again when it goes on.
pid=$(guest "grep 'ZWL IME started pid=' /tmp/zdesktop.log | tail -1" | sed -n 's/.*pid=\([0-9]*\).*/\1/p' | tail -1)
echo "input method: pid $pid"
guest "kill -STOP $pid" >/dev/null
keys 'k'
sleep 1.5
keys 'z'
expect_log /tmp/zdesktop.log 'ZWL IME bypass after_ms=500'
expect_log /tmp/p2.log 'PROBE KEY key=44 state=1'
guest "kill -CONT $pid" >/dev/null
sleep 1
expect_log /tmp/zdesktop.log 'ZWL IME answering'
keys '<esc>'

# 7. A killed input method is let go and started again.
guest "kill -KILL $pid" >/dev/null
expect_log /tmp/zdesktop.log 'ZWL IME lost'
expect_log /tmp/zdesktop.log 'ZWL IME started pid=' 2
expect_log /tmp/zdesktop.log 'KEI-IME READY languages=2' 2
expect_log /tmp/zdesktop.log 'ZWL IME activate client=' 3
keys 'a'
expect_log /tmp/p2.log 'PROBE KEY key=30 state=1'
shot restarted.png

# zdesktop saw no protocol error.
expect_none /tmp/zdesktop.log 'ZWL ERROR'
guest "grep -E 'ZWL IME|KEI-IME|ZWL ERROR' /tmp/zdesktop.log" > "$out/zdesktop-ime.txt"
guest "cat /tmp/p.log" > "$out/probe.txt"
guest "cat /tmp/p2.log" > "$out/probe2.txt"
echo "ime-p004: status=$status"
exit $status
