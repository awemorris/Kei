#!/bin/sh
# ws073-p015 on the amd64 hybrid image (UEFI, NVMe, KVM), from the host:
# boot0 (the BOOT FAT with rootfs.img, data.img and swapfile, 2 KiB
# clusters) is shown at /boot/boot0 and its files in use are protected.
# The image has no sshd, so boot-slots.sh is put on the BOOT FAT with mtools
# and run on the serial console (the image needs CONFIG_PCAT_SERIAL_MIRROR=y,
# plan/ws073/tests/config-amd64-hybrid-serial.mk).  After the guest stops,
# fsck.fat -n checks the BOOT FAT.
#
#   sh plan/ws073/tests/p015-hybrid.sh IMAGE [OUT]
#
# IMAGE is copied, never changed; OUT defaults to build/ws073-p015.
# Copyright (C) 2026 Awe Morris; SPDX-License-Identifier: Zlib
set -eu
# KVM when the host offers it, else TCG; QEMU_NO_KVM=1 forces TCG (ws129-p011).
. "$(dirname -- "$0")/../../tools/guest/qemu-accel.sh"
image=${1:?image}
out=${2:-build/ws073-p015}
here=$(cd "$(dirname "$0")" && pwd)
root=$(cd "$here/../../.." && pwd)
mkdir -p "$out"
disk=$out/hybrid.img
cp --reflink=auto "$image" "$disk"

# The BOOT FAT is the second GPT partition.
boot_start=$(/sbin/sfdisk -d "$disk" | awk '/img2 :/ { sub(",", "", $4); print $4 }')
boot_size=$(/sbin/sfdisk -d "$disk" | awk '/img2 :/ { sub(",", "", $6); print $6 }')
mcopy -o -i "$disk@@$((boot_start * 512))" "$here/boot-slots.sh" ::/SLOTS.SH

# Boots it with the serial console on a socket.
run=$out/hybrid-run
mkdir -p "$run"
rm -f "$run/serial.sock"
cp /usr/share/OVMF/OVMF_VARS_4M.fd "$run/vars.fd"
qemu-system-x86_64 -machine q35 -m 8G -smp 4 $(qemu_accel_args max) \
	-drive if=pflash,format=raw,readonly=on,file=/usr/share/OVMF/OVMF_CODE_4M.fd \
	-drive "if=pflash,format=raw,file=$run/vars.fd" \
	-drive "if=none,id=boot,file=$disk,format=raw" \
	-device nvme,serial=zedbsd-boot,drive=boot,bootindex=1 \
	-vga std -display none -no-reboot \
	-serial "unix:$run/serial.sock,server=on,wait=off" \
	-monitor none > "$run/qemu.log" 2>&1 &
pid=$!
trap 'kill "$pid" 2>/dev/null || true' EXIT
sleep 2
S="python3 $root/plan/tools/guest/serial.py --socket $run/serial.sock --timeout 300"
$S expect 'login: ' > /dev/null
$S login > /dev/null
status=0
$S run 'sh /boot/boot0/SLOTS.SH hybrid' || status=1
# The script stays: a FAT file that was read and then removed keeps its
# clusters until its inode goes (a separate bug), which fsck would report.
$S run 'sync' || status=1
$S run 'poweroff' > /dev/null 2>&1 || true
tries=0
while kill -0 "$pid" 2>/dev/null && [ $tries -lt 60 ]; do
	sleep 1
	tries=$((tries + 1))
done
kill "$pid" 2>/dev/null || true
wait "$pid" 2>/dev/null || true
trap - EXIT

# The BOOT FAT is clean on the host.
part=$out/boot-check.img
dd if="$disk" of="$part" bs=512 skip="$boot_start" count="$boot_size" status=none
if /sbin/fsck.fat -n "$part" > "$out/fsck-boot.txt" 2>&1; then
	echo "PASS host: fsck.fat -n BOOT clean"
else
	echo "FAIL host: fsck.fat -n BOOT"
	status=1
fi
rm -f "$part"
exit $status
