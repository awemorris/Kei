#!/bin/sh
# ws005-p025 (BUG-149): runs the poll probes on the lean amd64 image over the serial line.
# Copyright (C) 2026 Awe Morris; SPDX-License-Identifier: Zlib
#
# The image is the lean guest (plan/ws001/tests/config-amd64-lean-guest.mk, serial mirror) built with the
# probes of bug149-build.sh as /bin/bug149-poll and /bin/bug149-keiland (see plan/ws005/phase025/phase.md).
# The commands are run as root through plan/tools/guest/serial.py, and their own output and exit status are
# the evidence (the console log is not read).  The snapshot drive keeps the image unchanged.
#   sh plan/ws005/tests/bug149-check.sh IMAGE [RUN-DIRECTORY]
set -eu
# KVM when the host offers it, else TCG; QEMU_NO_KVM=1 forces TCG (ws129-p011).
. "$(dirname -- "$0")/../../tools/guest/qemu-accel.sh"
cd "$(dirname -- "$0")/../../.."
image=${1:?image}
run=${2:-build/p1-bug149-run}
code=${OVMF_CODE:-/usr/share/OVMF/OVMF_CODE_4M.fd}
vars=${OVMF_VARS:-/usr/share/OVMF/OVMF_VARS_4M.fd}
mkdir -p "$run"
rm -f "$run/serial.sock"
cp "$vars" "$run/vars.fd"
timeout 900 qemu-system-x86_64 -machine q35 -m 4G -smp 4 $(qemu_accel_args max) \
	-drive "if=pflash,format=raw,readonly=on,file=$code" \
	-drive "if=pflash,format=raw,file=$run/vars.fd" \
	-drive "if=none,id=boot,file=$image,format=raw,snapshot=on" \
	-device nvme,serial=zedbsd-boot,drive=boot,bootindex=1 \
	-vga std -display none -no-reboot \
	-serial "unix:$run/serial.sock,server=on,wait=off" \
	-monitor none < /dev/null > "$run/qemu.log" 2>&1 &
echo $! > "$run/qemu.pid"
trap 'kill "$(cat "$run/qemu.pid")" 2>/dev/null || true' EXIT

# Waits for the emulator to create the serial socket.
tries=0
while [ ! -S "$run/serial.sock" ] && [ "$tries" -lt 600 ]; do
	sleep 0.1
	tries=$((tries + 1))
done
serial() {
	python3 plan/tools/guest/serial.py --socket "$run/serial.sock" --timeout 120 "$@"
}
step() {
	echo "root\$ $1"
	if serial run "$1"; then
		echo "exit=0"
	else
		echo "exit=$?"
	fi
}
serial expect 'login: ' > /dev/null
serial login

# The kernel's poll after a write shutdown (and the cases that must not change).
step '/bin/bug149-poll'

# (d) the net command line: requests written, shut down for writing, then read with a bound.
step 'net show'
step 'net wifi list'
step 'net wifi disconnect'
step 'time net show > /dev/null'

# (b) zsv1-client (service): a nonblocking request with a write shutdown; its time tells a busy wait.
step 'time service list'
step 'time service status networkd'
step 'time service restart networkd'
step 'sleep 2; net show | head -3'

# (a) libkeiland against a stand-in daemon that answers a join after 3 seconds (replaces networkd's socket).
step '/bin/bug149-keiland'
