#!/bin/sh
# ws005-p019 (B3): checks that networkd keeps one default route and its resolver.
# Copyright (C) 2026 Awe Morris; SPDX-License-Identifier: Zlib
#
# QEMU gives the lean image (plan/ws001/tests/config-amd64-lean-guest.mk, serial
# mirror) two USB CDC ECM interfaces on two user networks (10.0.2.0/24 and
# 10.0.3.0/24, resolvers 10.0.2.3 and 10.0.3.3).  Before this Phase each
# lease's dhcpc added its own default route and the last resolver won.  With
# the preference in networkd exactly one default route is in the table, the
# first configured interface's, with that interface's resolver.  The script
# then removes that interface's device (QMP device_del) and expects the
# other's default route and resolver to be put in effect.  A Wi-Fi lease is
# handled by the same code (apply_network_preference); QEMU has no radio, so
# that half is checked on the 5330's AX211 passthrough.
#   sh plan/ws005/phase019/prefer-check.sh IMAGE [RUN-DIRECTORY]
set -eu
# KVM when the host offers it, else TCG; QEMU_NO_KVM=1 forces TCG (ws129-p011).
. "$(dirname -- "$0")/../../tools/guest/qemu-accel.sh"
image=${1:?image}
run=${2:-build/p1-prefer}
code=${OVMF_CODE:-/usr/share/OVMF/OVMF_CODE_4M.fd}
vars=${OVMF_VARS:-/usr/share/OVMF/OVMF_VARS_4M.fd}
mkdir -p "$run"
rm -f "$run/serial.sock" "$run/qmp.sock"
cp "$vars" "$run/vars.fd"
timeout 600 qemu-system-x86_64 -machine q35 -m 4G -smp 4 $(qemu_accel_args max) \
	-drive "if=pflash,format=raw,readonly=on,file=$code" \
	-drive "if=pflash,format=raw,file=$run/vars.fd" \
	-drive "if=none,id=boot,file=$image,format=raw,snapshot=on" \
	-device nvme,serial=zedbsd-boot,drive=boot,bootindex=1 \
	-device qemu-xhci,id=xhci \
	-netdev user,id=n0,net=10.0.2.0/24 -device usb-net,bus=xhci.0,netdev=n0,id=lan0 \
	-netdev user,id=n1,net=10.0.3.0/24 -device usb-net,bus=xhci.0,netdev=n1,id=lan1 \
	-vga std -display none -no-reboot \
	-serial "unix:$run/serial.sock,server=on,wait=off" \
	-qmp "unix:$run/qmp.sock,server=on,wait=off" \
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
qmp() {
	python3 - "$run/qmp.sock" "$1" <<'PY'
import json, socket, sys
qmp = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
qmp.connect(sys.argv[1])
reader = qmp.makefile("r")
reader.readline()
for command in ({"execute": "qmp_capabilities"}, json.loads(sys.argv[2])):
	qmp.sendall(json.dumps(command).encode() + b"\n")
	while True:
		reply = json.loads(reader.readline())
		if "return" in reply or "error" in reply:
			print("qmp:", json.dumps(reply))
			break
PY
}
serial expect 'login: ' > /dev/null
serial login

# Gives both leases time to finish, then shows the table and the resolver.
sleep 25
step 'ifconfig ue0; ifconfig ue1'
step 'route -n show'
step 'cat /etc/resolv.conf'

# Removes the interface whose resolver is in effect and shows the result.
if serial run 'grep -q "for ue0" /etc/resolv.conf' > /dev/null 2>&1; then
	gone=lan0
else
	gone=lan1
fi
echo "removing $gone"
qmp "{\"execute\": \"device_del\", \"arguments\": {\"id\": \"$gone\"}}"
sleep 15
step 'route -n show'
step 'cat /etc/resolv.conf'
step 'ping -c 1 10.0.2.2; ping -c 1 10.0.3.2'
