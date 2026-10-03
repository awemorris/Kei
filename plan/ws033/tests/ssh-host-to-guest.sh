#!/bin/sh
# WS033: OpenSSH from the host into the guest, over a USB-booted image.
# Copyright (C) 2026 Awe Morris; SPDX-License-Identifier: Zlib
#
# The other OpenSSH tests connect the guest to itself over the loopback,
# which exercises the protocol but not the network: nothing leaves the
# machine.  Here the image is booted from a USB stick and the guest is
# given a USB Ethernet adapter, so what is tested is the whole path -- the
# storage the system booted from, the network adapter on the same
# controller, the address it was given, and the server listening on it.
#
# Both devices hang off the one xHCI controller, which is how a machine
# with a stick in one port and an adapter in the other is arranged.
#
# QEMU's usb-net function offers RNDIS first and CDC Ethernet second.  The
# guest has no RNDIS and takes the second configuration, which is the case
# ws004's HW-T22 baseline covers; the interface it publishes is ue0.
#
# The session is interactive: ssh is given a terminal, so the guest
# allocates a pseudo terminal and the command runs under one, which is
# what a login does and what a bare command does not.
#
# usage: ssh-host-to-guest.sh [image]
set -eu
# KVM when the host offers it, else TCG; QEMU_NO_KVM=1 forces TCG (ws129-p011).
. "$(dirname -- "$0")/../../tools/guest/qemu-accel.sh"

root=$(cd "$(dirname "$0")/../../.." && pwd)
image=${1:-$root/build/amd64/hdd-image.img}
qemu=${QEMU_SYSTEM_X86_64:-qemu-system-x86_64}
# A port nobody else is on.  A test that picks a fixed one fails when
# another copy of itself is still finishing, which says nothing about the
# system under test.
port=${ZEDBSD_SSH_PORT:-$(python3 -c '
import socket
holder = socket.socket()
holder.bind(("127.0.0.1", 0))
print(holder.getsockname()[1])
holder.close()
')}

command -v "$qemu" >/dev/null
command -v ssh >/dev/null
test -f "$image"

work=$(mktemp -d "${TMPDIR:-/tmp}/zedbsd-ssh-host-XXXXXX")
disk=$work/usbstick.img
key=$work/id_ed25519
console=$work/console.log
monitor=$work/monitor.sock
pid=

cleanup() {
	if [ -n "$pid" ]; then
		kill "$pid" 2>/dev/null || true
		wait "$pid" 2>/dev/null || true
	fi
	if [ "${ZEDBSD_KEEP:-0}" = 1 ]; then
		echo "ssh-host-to-guest: kept $work" >&2
	else
		rm -rf "$work"
	fi
}
trap cleanup EXIT INT TERM

# The guest writes to the stick it booted from, so it is given a copy: a
# test must not change the image it was handed.
cp "$image" "$disk"

# The key the host will offer.  It is made here rather than carried in the
# tree, because a private key in a repository is a private key nobody has.
ssh-keygen -q -t ed25519 -N '' -C zedbsd-host-test -f "$key"

# -cpu max (TCG) or -cpu host (KVM, on a host that has it) is what gives the guest RDRAND.  Without it OpenSSL has nothing
# to seed from on this system and ssh-keygen refuses to make a host key,
# which is a gap in the system rather than in the test, but this test is
# about the network and runs the way the other guest tests run.
"$qemu" -machine q35 -m 512 -smp 2 $(qemu_accel_args max) \
	-device qemu-xhci,id=xhci \
	-drive "if=none,id=boot,file=$disk,format=raw" \
	-device usb-storage,bus=xhci.0,port=1,drive=boot,id=rootstick,bootindex=1 \
	-netdev "user,id=net0,net=10.0.2.0/24,host=10.0.2.2,dhcpstart=10.0.2.15,dns=10.0.2.3,hostfwd=tcp:127.0.0.1:$port-:22" \
	-device usb-net,bus=xhci.0,port=2,id=ecm,netdev=net0,mac=52:54:00:33:00:01,msos-desc=on \
	-boot c -display none -no-reboot \
	-serial "file:$console" \
	-qmp "unix:$monitor,server,nowait" >"$work/qemu.log" 2>&1 &
pid=$!

# Not exec: replacing this shell would throw away the trap above with it,
# and the emulator would be left running with nobody to stop it.
status=0
python3 "$root/plan/ws033/tests/ssh-host-to-guest.py" \
	--monitor "$monitor" --console "$console" --key "$key" \
	--interface ue0 --port "$port" --timeout 320 || status=$?
exit $status
