#!/bin/sh
# ws005-p019: checks the Wi-Fi control of the network group with the real networkd.
# Copyright (C) 2026 Awe Morris; SPDX-License-Identifier: Zlib
#
# Extends plan/ws005/phase018/owner-check.sh (the same QEMU harness) for the
# user decision of 2026-10-02: a member of the network group may control
# Wi-Fi whoever enabled it, and an explicit join moves the policy to the
# joining account.  The image is an amd64 image with CONFIG_PCAT_SERIAL_MIRROR=y
# (plan/ws001/tests/config-amd64-lean-guest.mk); QEMU gives it no radio.
#
# After the boot's `net startup` (root, so root owns the policy), kei (uid
# 1000, in network) saves a key in kei's store and joins: the join must pass
# the owner check and find the profile in kei's store, and stop only at
# "no WLAN radio" (ENODEV); root's store does not hold the profile, so ENOENT
# would mean the wrong store was read.  kei then turns Wi-Fi off (it was
# EPERM before), a join while off is refused as disabled, kei turns it on,
# disconnects and turns it off and on again.
# Last, an account outside the network group (made here, uid 1001) must be
# refused.  The SSID and passphrase are fixed fake test values.
#   sh plan/ws005/phase019/owner-check.sh IMAGE [RUN-DIRECTORY]
set -eu
image=${1:?image}
run=${2:-build/p1-owner}
code=${OVMF_CODE:-/usr/share/OVMF/OVMF_CODE_4M.fd}
vars=${OVMF_VARS:-/usr/share/OVMF/OVMF_VARS_4M.fd}
mkdir -p "$run"
rm -f "$run/serial.sock"
cp "$vars" "$run/vars.fd"
timeout 600 qemu-system-x86_64 -machine q35 -m 4G -smp 4 -cpu max -enable-kvm \
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
logout() {
	# Leaves the shell and waits for getty's prompt, pressing return now and
	# then: networkd's console lines can land after the shell's prompt.
	python3 - "$run/serial.sock" <<'PY'
import socket, sys, time
console = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
console.connect(sys.argv[1])
console.settimeout(1.0)
console.sendall(b"exit\r")
seen = b""
deadline = time.time() + 90
last = time.time()
while time.time() < deadline:
	try:
		seen += console.recv(4096)
	except socket.timeout:
		pass
	if seen.rstrip().endswith(b"login:"):
		sys.exit(0)
	if time.time() - last > 5:
		console.sendall(b"exit\r" if seen.rstrip()[-1:] in (b"#", b"$") else b"\r")
		last = time.time()
sys.stderr.write("logout: no login prompt; last output %r\n" % seen[-300:])
sys.exit(1)
PY
	sleep 1
}
step() {
	who=$1
	shift
	echo "$who\$ $1"
	if serial --user "$who" run "$1"; then
		echo "exit=0"
	else
		echo "exit=$?"
	fi
}
serial expect 'login: ' > /dev/null

# Makes kei's home (sessiond does it on a desktop) and an account outside the
# network group, guest, with kei's password hash (so it logs in with "kei").
serial login
step root 'mkdir -p /home/kei && chown kei:kei /home/kei && chmod 700 /home/kei'
step root 'echo guest:x:1001:1001:Guest:/home/guest:/bin/sh >> /etc/passwd'
step root 'echo guest:x:1001: >> /etc/group'
step root "sed -n 's/^kei:/guest:/p' /etc/shadow >> /etc/shadow"
step root 'mkdir -p /home/guest && chown 1001:1001 /home/guest && chmod 700 /home/guest'
step root 'net wifi list'
logout

serial --user kei login
step kei 'id'
step kei 'net wifi list'
step kei 'net wifi set-key P019-FAKE-SSID p019-fake-passphrase auto'
step kei 'net wifi connect P019-FAKE-SSID'
step kei 'net wifi disable'
step kei 'net wifi connect P019-FAKE-SSID'
step kei 'net wifi enable'
step kei 'net wifi list'
step kei 'net wifi disconnect'
step kei 'net wifi disable'
step kei 'net wifi enable'
logout

ZEDBSD_GUEST_PASSWORD=kei serial --user guest login
step guest 'id'
step guest 'net wifi list'
step guest 'net wifi disable'
step guest 'net wifi set-key P019-FAKE-SSID p019-fake-passphrase auto'
step guest 'net wifi connect P019-FAKE-SSID'
