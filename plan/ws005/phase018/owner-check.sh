#!/bin/sh
# ws005-p018: checks the Wi-Fi policy-owner mismatch with the real networkd.
# Copyright (C) 2026 Awe Morris; SPDX-License-Identifier: Zlib
#
# Boots an amd64 image built with CONFIG_PCAT_SERIAL_MIRROR=y (for example
# plan/ws001/tests/config-amd64-lean-guest.mk) in QEMU without any radio,
# logs in on the serial console as kei (the boot's `net startup` ran as root
# and made root the policy owner) and runs the commands a desktop join sends:
# save a key in kei's store, notify, connect; then the off/on the Settings
# message advises; then `net wifi enable` as kei and connect again.  Without
# a radio the last connect is expected to stop at "no WLAN radio" (ENODEV),
# which is past the owner check and the profile lookup.  The SSID
# and passphrase are fixed fake test values, not a real network's.
#   sh plan/ws005/phase018/owner-check.sh IMAGE [RUN-DIRECTORY]
set -eu
# KVM when the host offers it, else TCG; QEMU_NO_KVM=1 forces TCG (ws129-p011).
. "$(dirname -- "$0")/../../tools/guest/qemu-accel.sh"
image=${1:?image}
run=${2:-build/ws005-p018-owner}
code=${OVMF_CODE:-/usr/share/OVMF/OVMF_CODE_4M.fd}
vars=${OVMF_VARS:-/usr/share/OVMF/OVMF_VARS_4M.fd}
mkdir -p "$run"
rm -f "$run/serial.sock"
cp "$vars" "$run/vars.fd"
timeout 600 qemu-system-x86_64 -machine q35 -m 4G -smp 4 $(qemu_accel_args max) \
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
while [ ! -S "$run/serial.sock" ] && [ "$tries" -lt 100 ]; do
	sleep 0.1
	tries=$((tries + 1))
done
s="python3 plan/tools/guest/serial.py --socket $run/serial.sock --timeout 120"
$s expect 'login: ' > /dev/null

# A desktop login makes kei's home (sessiond); this image has no desktop, so
# root makes it once, then the console goes back to the login prompt.
$s login
$s run 'mkdir -p /home/kei && chown kei:kei /home/kei && chmod 700 /home/kei'
python3 plan/tools/guest/serial.py --socket "$run/serial.sock" --timeout 5 \
	run 'exit' > /dev/null 2>&1 || true
sleep 3
$s --user kei login
status=0
for command in \
	'id' \
	'net wifi list' \
	'net wifi add P018-FAKE-SSID --password p018-fake-passphrase --auto yes' \
	'net wifi connect P018-FAKE-SSID' \
	'net wifi disable' \
	'net wifi enable' \
	'net wifi connect P018-FAKE-SSID' \
	'net wifi disable'; do
	echo "kei\$ $command"
	$s run "$command" || echo "exit=$?"
done
exit $status
