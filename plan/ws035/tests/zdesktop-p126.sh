#!/bin/sh
# ws035-p126: the black between the greeter and the session (F-048), on the Venus guest of the graphical login image
# (plan/ws035/tests/build-login-image.sh BUILD graphical; the image logs the user kei in by itself at boot).
# Each cycle photographs the screen several times a second (frames.py: black, text or picture) across
#  1. Log Out: App Home's Log Out ends kei's session and sessiond shows the greeter (SESSIOND GREETER adopt), and
#  2. Login: kei's password and Enter at the greeter log kei in again (SESSIOND AUTH ok user=kei, SESSIOND HANDOFF go written).
# Every hand-over is reported with its black pictures and its black time (from the first black picture to the next
# picture that is not black; the pictures are about 0.2 s apart).  The run fails when a picture is the text console
# or a step of the hand-over is missing; black pictures are reported, and fail the run only with --no-black.
# The pictures are kept in OUTDIR/logout-N and OUTDIR/login-N, the lists in OUTDIR/logout-N.txt and OUTDIR/login-N.txt.
#
#   GUEST_RUNTIME=build/ws035-run plan/ws035/tests/zdesktop-guest.sh start build/amd64/hdd-image.img
#   GUEST_RUNTIME=build/ws035-run plan/ws035/tests/zdesktop-p126.sh [OUTDIR] [CYCLES] [--no-black]
# Copyright (C) 2026 Awe Morris; SPDX-License-Identifier: Zlib
set -u
cd "$(dirname -- "$0")/../../.."
GUEST_RUNTIME="${GUEST_RUNTIME:-$(pwd)/build/ws035-run}"
export GUEST_RUNTIME
out=${1:-build/ws035-p126}
cycles=${2:-3}
strict=${3:-}
mkdir -p "$out"
# The SSH to the guest, tried again when ssh itself fails (plan/ws099/tests/guest-retry.sh, ws099-p023).
. plan/ws099/tests/guest-retry.sh
guest() { guest_retry 120 "$1" </dev/null; }
keys() { python3 plan/ws035/tests/qmp-keys.py "$GUEST_RUNTIME/qmp.sock" "$@"; }
pointer() { python3 plan/ws035/tests/qmp-pointer.py --width 1280 --height 800 "$GUEST_RUNTIME/qmp.sock" "$@"; }
frames() { python3 plan/ws035/tests/frames.py "$out/$1" --runtime "$GUEST_RUNTIME" --seconds "$2" > "$out/$1.txt" 2>&1; }
status=0
total_black=0

# Waits until a guest file has at least COUNT lines matching a pattern (within some seconds).
expect_count() {
	tries=0
	found=0
	while [ $tries -lt "$4" ]; do
		found=$(guest "grep -cE '$2' $1" | tail -1)
		[ "${found:-0}" -ge "$3" ] 2>/dev/null && break
		tries=$((tries + 1))
		sleep 1
	done
	if [ "${found:-0}" -ge "$3" ] 2>/dev/null; then
		echo "log: $2 ($3) ok"
	else
		echo "log: $2 ($3) MISSING"
		status=1
	fi
}

# Reports one hand-over's pictures: the text console fails the run, black pictures are counted and timed.
report() {
	set -- "$1" $(awk '
		/frame-/ { pictures++ }
		/ text$/ { text++ }
		/ black$/ { black++; if (start == "") start = $1 }
		/frame-/ && !/ black$/ { if (start != "" && end == "") end = $1 }
		END { span = 0; if (start != "") { if (end == "") end = start; span = end - start }
		      printf "%d %d %d %d\n", pictures, text, black, span }' "$out/$1.txt")
	echo "frames: $1: $2 pictures, text $3, black $4, black time ${5} ms"
	[ "$2" -gt 0 ] || { echo "frames: $1: no pictures FAIL"; status=1; }
	[ "$3" -eq 0 ] || { echo "frames: $1: the text console was shown FAIL"; status=1; }
	total_black=$((total_black + $4))
}

# The session kei was logged in at boot.
expect_count /var/log/sessiond.log 'SESSIOND HANDOFF go written=3' 1 90
sleep 4
cycle=1
while [ $cycle -le "$cycles" ]; do
	# 1. Log Out, from App Home.
	pointer move 23 17 sleep 300 down sleep 60 up sleep 1500
	set -- $(guest "grep 'ZWL HOME icon name=\"Log Out\"' /run/user/1000/session.log | tail -1" | sed -n 's/.* x=\([0-9]*\) y=\([0-9]*\).*/\1 \2/p')
	if [ -z "${1:-}" ]; then
		echo "cycle $cycle: no Log Out icon"
		status=1
		break
	fi
	frames logout-$cycle 12 &
	sleep 1
	pointer move "$1" "$2" sleep 400 down sleep 60 up
	wait
	expect_count /var/log/sessiond.log 'SESSIOND GREETER adopt pid=' "$cycle" 10
	report logout-$cycle

	# 2. Login at the greeter: kei's password ("kei", the demonstration's accounts) and Enter.
	sleep 2
	frames login-$cycle 12 &
	sleep 1
	keys 'kei\n'
	wait
	expect_count /var/log/sessiond.log 'SESSIOND AUTH ok user=kei' "$cycle" 10
	expect_count /var/log/sessiond.log 'SESSIOND HANDOFF go written=3' $((cycle + 1)) 10
	report login-$cycle
	sleep 4
	cycle=$((cycle + 1))
done

# The kernel's own account of the hand-overs (the Venus driver's hold, ws035-p126), and sessiond's.
guest "dmesg | grep -E 'venus: (lease released|the next lease|the held picture)'" > "$out/dmesg-hold.txt"
guest "cat /var/log/sessiond.log" > "$out/sessiond.log"
echo "black pictures in all hand-overs: $total_black"
if [ "$strict" = --no-black ] && [ "$total_black" -ne 0 ]; then
	echo "black pictures with --no-black FAIL"
	status=1
fi
echo "zdesktop-p126: status=$status"
exit $status
