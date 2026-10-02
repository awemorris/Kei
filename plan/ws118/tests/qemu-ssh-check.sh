#!/bin/sh
# ws118-p001: boots a remote-log image from USB in QEMU and collects its logs over SSH.
# Copyright (C) 2026 Awe Morris; SPDX-License-Identifier: Zlib
#
#   plan/ws118/tests/qemu-ssh-check.sh IMAGE PORT OUTDIR
#
# The 5320's arrangement in QEMU (the WS033 ssh-host-to-guest.sh form, with
# UEFI): the image on a USB stick and a USB Ethernet adapter (usb-net, CDC
# Ethernet, ue0) on one xHCI controller, QEMU's user network giving DHCP,
# and the guest's port 22 forwarded to 127.0.0.1:PORT.  The guest gets a
# copy of the image (OUTDIR/usbstick.img, removed at the end unless
# KEEP=1).  When sshd answers, plan/ws118/tests/collect-5320.sh runs against
# it into OUTDIR/logs.  Then the guest is asked to sync and is powered off
# (QMP quit), and the stick is kept for the disk-log check when KEEP=1.
# This is QEMU evidence, not the 5320's.
set -u
cd "$(dirname -- "$0")/../../.."
image=${1:?image}
port=${2:?port}
out=${3:?outdir}
code=${OVMF_CODE:-/usr/share/OVMF/OVMF_CODE_4M.fd}
vars=${OVMF_VARS:-/usr/share/OVMF/OVMF_VARS_4M.fd}
key=plan/tmp/guest/id_ed25519
mkdir -p "$out"
rm -f "$out/qmp.sock"
cp --sparse=always "$image" "$out/usbstick.img"
cp "$vars" "$out/vars.fd"
timeout 900 qemu-system-x86_64 -machine q35,accel=kvm -m 4G -smp 4 -cpu max \
	-drive "if=pflash,format=raw,readonly=on,file=$code" \
	-drive "if=pflash,format=raw,file=$out/vars.fd" \
	-device qemu-xhci,id=xhci \
	-drive "if=none,id=boot,file=$out/usbstick.img,format=raw" \
	-device usb-storage,bus=xhci.0,port=1,drive=boot,id=rootstick,bootindex=1 \
	-netdev "user,id=net0,net=10.0.2.0/24,host=10.0.2.2,dhcpstart=10.0.2.15,dns=10.0.2.3,hostfwd=tcp:127.0.0.1:$port-:22" \
	-device usb-net,bus=xhci.0,port=2,netdev=net0 \
	-vga std -display none -serial none -monitor none -no-reboot \
	-qmp "unix:$out/qmp.sock,server=on,wait=off" < /dev/null > "$out/qemu.log" 2>&1 &
pid=$!
quit() {
	python3 - "$out/qmp.sock" <<'PY' > /dev/null 2>&1
import json, socket, sys
qmp = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
qmp.connect(sys.argv[1])
reader = qmp.makefile("r")
reader.readline()
for command in ({"execute": "qmp_capabilities"}, {"execute": "quit"}):
	qmp.sendall(json.dumps(command).encode() + b"\n")
	reader.readline()
PY
	sleep 2
	kill "$pid" 2>/dev/null
	wait "$pid" 2>/dev/null
	[ "${KEEP:-0}" = 1 ] || rm -f "$out/usbstick.img"
}
trap quit EXIT

# Waits for sshd to answer with the key (OpenSSH makes its host keys on the first boot).
started=$(date +%s)
answered=0
while [ $(( $(date +%s) - started )) -lt 600 ]; do
	if timeout 15 ssh -n -i "$key" -p "$port" -o BatchMode=yes -o ConnectTimeout=5 \
		-o StrictHostKeyChecking=no -o UserKnownHostsFile=/dev/null -o LogLevel=ERROR \
		root@127.0.0.1 true 2> /dev/null; then
		answered=1
		break
	fi
	sleep 5
done
echo "qemu-ssh-check: sshd answered=$answered after $(( $(date +%s) - started )) s"
[ "$answered" = 1 ] || exit 1
SSH_PORT=$port plan/ws118/tests/collect-5320.sh 127.0.0.1 "$out/logs"

# Waits up to 90 s for root's crontab to have copied the kernel's messages to the disk.
for i in $(seq 1 18); do
	timeout 15 ssh -n -i "$key" -p "$port" -o BatchMode=yes -o StrictHostKeyChecking=no \
		-o UserKnownHostsFile=/dev/null -o LogLevel=ERROR root@127.0.0.1 \
		'test -s /var/log/dmesg.cron' 2> /dev/null && break
	sleep 5
done

# Flushes the guest's disk before QEMU is quit, for the disk-log check.
timeout 30 ssh -n -i "$key" -p "$port" -o BatchMode=yes -o StrictHostKeyChecking=no \
	-o UserKnownHostsFile=/dev/null -o LogLevel=ERROR root@127.0.0.1 \
	'logger ws118-p001-disk-log-marker; sleep 2; sync' > /dev/null 2>&1
