#!/bin/sh
# ws005-p019: drives the desktop image on the 5330 with the iGPU and the AX211 passed through, from this host.
# Copyright (C) 2026 Awe Morris; SPDX-License-Identifier: Zlib
#
#   plan/ws005/phase019/wifi-desktop-hw.sh start IMAGE OUTDIR   takes the machine (/tmp/i915-hw.lock, through
#                                                               plan/ws075/tests/hdmi/h4-lock.sh, as hdmi-h4-hw.sh
#                                                               does), copies the image, wifi-desktop-qemu.sh and
#                                                               h4-ctl.py to ~/bigbang/h4/ and starts QEMU
#   plan/ws005/phase019/wifi-desktop-hw.sh ctl ARGS...          one h4-ctl.py command (shot, pointer, keys, hmp, quit)
#   plan/ws005/phase019/wifi-desktop-hw.sh stop OUTDIR          quits QEMU, waits for the AX211 to be back on
#                                                               iwlwifi (restore.log), fetches the shots and gives
#                                                               the machine back (hdmi-h4-hw.sh stop)
# Do not bound start with timeout (see hdmi-h4-hw.sh).  I915_HOST names the 5330 (default solaris10-man).
set -u
cd "$(dirname -- "$0")/../../.."
host=${I915_HOST:-solaris10-man}
remote=bigbang/h4
owner=/tmp/i915-h4-owner
h4=plan/ws075/tests/hdmi-h4-hw.sh
command=${1:-}
[ $# -gt 0 ] && shift
case "$command" in
start)
	image=$1
	out=$2
	mkdir -p "$out"
	[ -f "$out/.running" ] && { echo "wifi-desktop-hw: $out is running"; exit 1; }
	touch "$out/.running"
	rm -f "$out/.locked"
	setsid nohup plan/ws075/tests/hdmi/h4-lock.sh "$out" < /dev/null > /dev/null 2>&1 &
	until [ -f "$out/.locked" ]; do sleep 1; done
	(cd "$out" && pwd) > "$owner"
	ssh "$host" bigbang/igpu-mode.sh vfio > /dev/null || { echo "iGPU is not on vfio-pci"; rm -f "$out/.running"; exit 1; }
	ssh "$host" "mkdir -p $remote && sudo -n rm -rf $remote/shots $remote/restore.log"
	scp -q "$image" "$host:$remote/guest.img" || { rm -f "$out/.running"; exit 1; }
	scp -q plan/ws005/phase019/wifi-desktop-qemu.sh plan/ws075/tests/hdmi/h4-ctl.py "$host:$remote/" || { rm -f "$out/.running"; exit 1; }
	ssh -n -f "$host" "cd $remote && H4_MINUTES=${H4_MINUTES:-60} setsid nohup bash wifi-desktop-qemu.sh /home/awe/$remote/guest.img < /dev/null > /dev/null 2>&1 &"
	echo "wifi-desktop-hw: QEMU started on $host at $(date '+%H:%M:%S')"
	;;
ctl)
	exec "$h4" ctl "$@"
	;;
stop)
	out=$1
	"$h4" ctl quit > /dev/null 2>&1
	# The AX211 goes back to iwlwifi when QEMU ends; the machine is not given back before that.
	for i in $(seq 1 60); do
		ssh "$host" "test -f $remote/restore.log" && break
		sleep 2
	done
	ssh "$host" "cat $remote/restore.log" | tee "$out/restore.log"
	"$h4" stop "$out"
	;;
*)
	echo "usage: $0 start IMAGE OUTDIR | ctl ARGS... | stop OUTDIR"
	exit 2
	;;
esac
