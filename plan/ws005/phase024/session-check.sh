#!/bin/sh
# ws005-p024: checks the login-session notices of the Wi-Fi stores with the real networkd.
# Copyright (C) 2026 Awe Morris; SPDX-License-Identifier: Zlib
#
# The user decided on 2026-10-02 that the boot joins from the system's store
# only, and that sessiond tells networkd when a session opens and closes, so
# an account's own store is a candidate only while it is logged in.  This
# drives "net wifi session open|close [account]" (what sessiond runs) on the
# lean amd64 image (plan/ws001/tests/config-amd64-lean-guest.mk, serial
# mirror, no radio) and reads networkd's status line ("wifi state=... owner=
# store= sessions=").  Root names any account; a member of the network group
# names only itself; an account outside the group is kept out by the socket;
# an account the database does not know is refused.  No radio exists, so the
# joining itself is checked on the 5330 (wifi-desktop-hw.sh), not here.
#   sh plan/ws005/phase024/session-check.sh IMAGE [RUN-DIRECTORY]
set -eu
image=${1:?image}
run=${2:-build/p1-session}
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

# An account outside the network group, guest, with kei's password hash, and kei's home.
serial login
step root 'mkdir -p /home/kei && chown kei:kei /home/kei && chmod 700 /home/kei'
step root 'echo guest:x:1001:1001:Guest:/home/guest:/bin/sh >> /etc/passwd'
step root 'echo guest:x:1001: >> /etc/group'
step root "sed -n 's/^kei:/guest:/p' /etc/shadow >> /etc/shadow"
step root 'mkdir -p /home/guest && chown 1001:1001 /home/guest && chmod 700 /home/guest'

# Root (as sessiond): the boot's policy is root's and no session is open.
step root 'net wifi list | grep "^wifi "'
step root 'net wifi session open kei'
step root 'net wifi list | grep "^wifi "'
step root 'net wifi session open root'
step root 'net wifi list | grep "^wifi "'
step root 'net wifi session close root'
step root 'net wifi list | grep "^wifi "'
step root 'net wifi session open 4242'
step root 'net wifi session close kei'
step root 'net wifi list | grep "^wifi "'
logout

# kei (network group): only kei's own session.
serial --user kei login
step kei 'id'
step kei 'net wifi session open'
step kei 'net wifi list | grep "^wifi "'
step kei 'net wifi session open root'
step kei 'net wifi session close guest'
step kei 'net wifi list | grep "^wifi "'
step kei 'net wifi add P024-FAKE-SSID --password p024-fake-passphrase --auto yes'
step kei 'net wifi connect P024-FAKE-SSID'
step kei 'net wifi session close'
step kei 'net wifi list | grep "^wifi "'
logout

# guest (outside the group): the socket refuses it.
ZEDBSD_GUEST_PASSWORD=kei serial --user guest login
step guest 'id'
step guest 'net wifi session open'
logout
