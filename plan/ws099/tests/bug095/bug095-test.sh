#!/bin/sh
# BUG-095 (ws099-p027): the oneshot service bug095_poweroff runs /sbin/poweroff 30 seconds after the boot; the
# machine must turn off.  Before the fix init waited for the oneshot while /sbin/poweroff waited for init's answer,
# and the emulator ran on.  Judged by the emulator's process alone (the guest's console is not read): it must have
# ended within LIMIT seconds (default 180) of the start.  The guest answering SSH before it ends is noted too.
#
#   plan/ws099/tests/bug095/bug095-test.sh IMAGE [LIMIT]
# Prints "bug095: PASS" or "bug095: FAIL".
# Copyright (C) 2026 Awe Morris; SPDX-License-Identifier: Zlib
set -u
cd "$(dirname -- "$0")/../../../.."
image=${1:?usage: bug095-test.sh IMAGE [LIMIT]}
limit=${2:-180}
GUEST_RUNTIME="${GUEST_RUNTIME:-$(pwd)/build/ws099-bug095-run}"
export GUEST_RUNTIME
python3 plan/tools/guest/guest.py stop >/dev/null 2>&1
python3 plan/tools/guest/guest.py start "$image" || { echo "bug095: FAIL (the guest did not start)"; exit 1; }
pid=$(python3 -c 'import json,sys; print(json.load(open(sys.argv[1]))["pid"])' "$GUEST_RUNTIME/session.json")
began=$(date +%s)
answered=no
while kill -0 "$pid" 2>/dev/null; do
	now=$(date +%s)
	[ $((now - began)) -ge "$limit" ] && break
	if [ $answered = no ] && timeout 10 python3 plan/tools/guest/guest.py run true >/dev/null 2>&1; then
		answered=yes
		echo "guest answered SSH after $(($(date +%s) - began)) s"
	fi
	sleep 2
done
waited=$(($(date +%s) - began))
if kill -0 "$pid" 2>/dev/null; then
	echo "emulator still running after $waited s"
	python3 plan/tools/guest/guest.py stop >/dev/null 2>&1
	echo "bug095: FAIL"
	exit 1
fi
echo "emulator ended after $waited s (answered SSH: $answered)"
tail -3 "$GUEST_RUNTIME/qemu.log" 2>/dev/null
python3 plan/tools/guest/guest.py stop >/dev/null 2>&1
echo "bug095: PASS"
