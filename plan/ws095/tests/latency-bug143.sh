#!/bin/sh
# ws095-p014 (BUG-143): how long a typed key takes to show, on the Venus guest of the Settings image with the input
# method (SETTINGS_CONFIG=plan/ws089/tests/config-amd64-settings-ime.mk plan/ws089/tests/build-settings-image.sh BUILD).
# zdesktop --glass at 1280x800 starts keiland-ime (direct input, "A", at first).  The run (type-latency.py):
#  1. textedit-direct: Text Editor on a file whose first line is long, the cursor at its start; each "x" typed shifts
#     the whole line, so the region of the line changes by far more than the cursor's blink (threshold 300 pixels).
#  2. terminal-direct: the same in Terminal (the line of the shell; threshold 20) as the baseline.
#  3. textedit-japanese: Alt+Space (Japanese), then for each trial "kanji", Space and Enter (a conversion learned and
#     saved to the user dictionary), then "a" measured (its preedit あ); and the session's ZWL IME bypass lines are counted (before
#     the fix the save on Enter held the input method past 500 ms: "ZWL IME bypass after_ms=500").
# Reported, not judged against a number for 1 and 2 (the guest's own figures are the evidence; 5330 is measured by the
# user).  PASS needs every trial shown (missed=0) and no "ZWL IME bypass" in step 3.
#
#   plan/ws089/tests/settings-guest.sh start IMAGE      (or zdesktop-guest.sh start IMAGE)
#   plan/ws095/tests/latency-bug143.sh [OUTDIR]          (default build/ws095-shots/bug143)
# Copyright (C) 2026 Awe Morris; SPDX-License-Identifier: Zlib
set -u
cd "$(dirname -- "$0")/../../.."
GUEST_RUNTIME="${GUEST_RUNTIME:-$(pwd)/build/ws089-run}"
export GUEST_RUNTIME
out=${1:-build/ws095-shots/bug143}
mkdir -p "$out"
guest() { timeout 90 python3 plan/tools/guest/guest.py run "$1" 2>&1 </dev/null; }
keys() { python3 plan/ws035/tests/qmp-keys.py "$GUEST_RUNTIME/qmp.sock" "$@"; sleep 0.5; }
latency() { python3 plan/ws095/tests/type-latency.py --runtime "$GUEST_RUNTIME" "$@" | tee -a "$out/latency.txt"; }
status=0
. plan/ws089/tests/settings-wait.sh

# Fails the run unless a log has a line matching a pattern (within a few seconds).
expect_log() {
	tries=0
	found=0
	while [ $tries -lt 10 ]; do
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
: > "$out/latency.txt"

# 0. zdesktop with the input method; a file with a long first line.
guest 'service stop greeter >/dev/null 2>&1; for p in $(ps -A -o pid,args | grep -E "[w]ayland( |$)|[k]eiland-ime|[t]extedit|[t]erminal" | awk "{print \$1}"); do kill $p; done; sleep 1; echo stopped' >/dev/null
guest 'export XDG_RUNTIME_DIR=/tmp HOME=/root; rm -f /tmp/wayland-0 /root/.config/kei/ime/ja-user.dict; picture=; [ -f /usr/share/keiland/wallpaper.ppm ] && picture=--wallpaper=/usr/share/keiland/wallpaper.ppm
/bin/wayland --timeout=900 --width=1280 --height=800 --glass $picture > /tmp/zdesktop.log 2>&1 </dev/null & sleep 4; echo started' >/dev/null
expect_log /tmp/zdesktop.log 'KEI-IME READY'
guest 'i=0; : > /root/lat.txt; while [ $i -lt 30 ]; do printf "the quick brown fox jumps over the lazy dog %d\n" $i >> /root/lat.txt; i=$((i+1)); done; echo made' >/dev/null

# 1. Text Editor, direct input: the first line's region (the window's text area, its first rows).
guest 'export XDG_RUNTIME_DIR=/tmp HOME=/root; /bin/textedit /root/lat.txt > /tmp/te.log 2>&1 </dev/null & sleep 5; echo started' >/dev/null
expect_log /tmp/te.log 'TEXTEDIT READY'
find_window
echo "textedit at $wx,$wy"
latency --name textedit-direct --key x --region "$((wx + 20)),$((wy + 10)),700,60" --threshold 300 --trials 10

# 2. Terminal, direct input (the baseline): Text Editor closed first.
guest "pid=\$(ps -A -o pid,args | grep '[t]extedit' | awk '{print \$1}'); kill \$pid" >/dev/null
sleep 1
guest 'export XDG_RUNTIME_DIR=/tmp HOME=/root; cd /root; /bin/terminal --token=lat --timeout-s=600 > /tmp/t.log 2>&1 </dev/null & sleep 6; echo started' >/dev/null
expect_log /tmp/t.log 'ZTERM START run=lat'
find_window
echo "terminal at $wx,$wy"
latency --name terminal-direct --key x --region "$((wx)),$((wy)),700,120" --threshold 20 --trials 10
guest "pid=\$(ps -A -o pid,args | grep '[t]erminal' | awk '{print \$1}'); kill \$pid" >/dev/null
sleep 1

# 3. Text Editor in Japanese: a conversion learned (and saved) before each measured key.
bypass_before=$(guest "grep -c 'ZWL IME bypass' /tmp/zdesktop.log" | tail -1)
guest 'export XDG_RUNTIME_DIR=/tmp HOME=/root; /bin/textedit /root/lat.txt > /tmp/te2.log 2>&1 </dev/null & sleep 5; echo started' >/dev/null
expect_log /tmp/te2.log 'TEXTEDIT READY'
find_window
keys '<alt-spc>'
expect_log /tmp/zdesktop.log 'ZWL IME language=ja'
for round in 1 2 3 4 5; do
	keys 'kanji' ' ' ' ' '\n'
	latency --name textedit-japanese-$round --key a --region "$((wx + 20)),$((wy + 10)),700,60" --threshold 300 --trials 1 --pause 200
	keys '\n'
done
bypass_after=$(guest "grep -c 'ZWL IME bypass' /tmp/zdesktop.log" | tail -1)
echo "ZWL IME bypass lines: ${bypass_before:-?} -> ${bypass_after:-?}"
[ "${bypass_after:-1}" = "${bypass_before:-0}" ] && echo "no IME bypass: ok" || { echo "no IME bypass: FAIL"; status=1; }
guest 'grep -E "ZWL IME|KEI-IME" /tmp/zdesktop.log' > "$out/zdesktop-ime.log"
grep -q 'missed=[1-9]' "$out/latency.txt" && { echo "a trial was never shown: FAIL"; status=1; }
grep '^LATENCY' "$out/latency.txt"
[ $status = 0 ] && echo "latency-bug143: PASS" || echo "latency-bug143: FAIL"
exit $status
