#!/bin/sh
# Boots an amd64 image (UEFI, NVMe, 8 GiB) in QEMU, logs in on the
# serial console and runs each argument as one command, printing its output.
# The image must be built with CONFIG_PCAT_SERIAL_MIRROR=y.  The disk is
# opened with snapshot=on, so the image is left as it was.
#   sh plan/tools/guest/amd64-serial.sh build/amd64/hdd-image.img 'uname -a'
# RUN is the work directory (default build/amd64-serial-run).
set -e
# KVM when the host offers it, else TCG; QEMU_NO_KVM=1 forces TCG (ws129-p011).
. "$(dirname -- "$0")/qemu-accel.sh"
IMAGE=${1:?image}
shift
D=${RUN:-build/amd64-serial-run}
CODE=${OVMF_CODE:-/usr/share/OVMF/OVMF_CODE_4M.fd}
VARS=${OVMF_VARS:-/usr/share/OVMF/OVMF_VARS_4M.fd}
mkdir -p "$D"
rm -f "$D/serial.sock"
cp "$VARS" "$D/vars.fd"
qemu-system-x86_64 -machine q35 -m 8G -smp 4 $(qemu_accel_args max) \
  -drive "if=pflash,format=raw,readonly=on,file=$CODE" \
  -drive "if=pflash,format=raw,file=$D/vars.fd" \
  -drive "if=none,id=boot,file=$IMAGE,format=raw,snapshot=on" \
  -device nvme,serial=zedbsd-boot,drive=boot,bootindex=1 \
  -vga std -display none -no-reboot \
  -serial "unix:$D/serial.sock,server=on,wait=off" \
  -monitor none > "$D/qemu.log" 2>&1 &
echo $! > "$D/qemu.pid"
trap 'kill "$(cat "$D/qemu.pid")" 2>/dev/null || true' EXIT
# QEMU opens a snapshot=on drive before it makes the serial socket; on a busy host that takes 8–12 s (ws115-p005).
i=0
while [ ! -S "$D/serial.sock" ] && [ $i -lt 600 ]; do sleep 0.1; i=$((i + 1)); done
S="python3 plan/tools/guest/serial.py --socket $D/serial.sock --timeout ${TIMEOUT:-180}"
$S expect 'login: ' > /dev/null
echo "amd64: login prompt"
$S login
status=0
for command in "$@"; do
	echo "amd64\$ $command"
	$S run "$command" || status=$?
done
exit $status
