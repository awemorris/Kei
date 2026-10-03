#!/bin/sh
# WS131 p003: the dedicated FreeBSD 15.1 QEMU guest for the native Keiland build (the WS109 guest, recreated in the
# worktree's own build/ after the main checkout's build/ws109-control/ was removed with an old build/; 2026-10-03 user
# "guest を作り直す（推奨）").  The guest is reached only through 127.0.0.1's forwarded SSH port and QMP; its serial
# and console are not read (AGENTS.md, WS109 exception).  No device is passed through.
#
#   sh plan/ws131/tools/freebsd-guest.sh fetch      download the official image, check its SHA256, unpack the base
#   sh plan/ws131/tools/freebsd-guest.sh seed       make the SSH key and the NoCloud seed (root by key only)
#   sh plan/ws131/tools/freebsd-guest.sh start      start the guest on a fresh overlay (OVERLAY=keep keeps the old one;
#                                                   FIRMWARE=uefi boots through OVMF instead of SeaBIOS)
#   sh plan/ws131/tools/freebsd-guest.sh ssh CMD    run CMD in the guest as root
#   sh plan/ws131/tools/freebsd-guest.sh wait       wait (at most WAIT seconds, 600) until SSH answers
#   sh plan/ws131/tools/freebsd-guest.sh shot PNG   photograph the guest's screen through QMP
#   sh plan/ws131/tools/freebsd-guest.sh stop       ask QEMU to quit through QMP
#
# 2026-10-03: the first boot on SeaBIOS stopped after the kernel's messages, the second stayed at "Loading kernel...";
# the user then postponed the FreeBSD tests (the host's memory).  -cpu host and FIRMWARE=uefi are the next things to try.
# Do not start the guest until the user resumes the FreeBSD tests.
#
# Everything lives in $DIR (default build/ws109-control of the current repository).  Copyright (C) 2026 Awe Morris;
# SPDX-License-Identifier: Zlib
set -eu
cd "$(dirname -- "$0")/../../.."
DIR=${DIR:-build/ws109-control}
PORT=${PORT:-47969}
URL=https://download.freebsd.org/releases/VM-IMAGES/15.1-RELEASE/amd64/Latest/FreeBSD-15.1-RELEASE-amd64-BASIC-CLOUDINIT-ufs.qcow2.xz
SHA256=e4ca4db889f8559c9b9dfcacc70405c038476f4b6d41649b152d3809a2ed9e1f
BASE=$DIR/freebsd15.1.qcow2
GUEST=$DIR/guest
KEY=$DIR/key/id_ed25519
SSH="ssh -i $KEY -p $PORT -o StrictHostKeyChecking=no -o UserKnownHostsFile=/dev/null -o ConnectTimeout=10 -o BatchMode=yes -o LogLevel=ERROR root@127.0.0.1"

qmp() {
	python3 - "$GUEST/qmp.sock" "$@" <<'PY'
import json, socket, sys
sock = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
sock.settimeout(20)
sock.connect(sys.argv[1])
stream = sock.makefile('rw')
json.loads(stream.readline())
def call(command, arguments=None):
    message = {'execute': command}
    if arguments:
        message['arguments'] = arguments
    stream.write(json.dumps(message) + '\n')
    stream.flush()
    while True:
        answer = json.loads(stream.readline())
        if 'return' in answer or 'error' in answer:
            return answer
call('qmp_capabilities')
if sys.argv[2] == 'shot':
    print(call('screendump', {'filename': sys.argv[3], 'format': 'png'}))
elif sys.argv[2] == 'quit':
    print(call('quit'))
PY
}

case "${1:-}" in
fetch)
	mkdir -p "$DIR"
	if [ ! -f "$DIR/image.qcow2.xz" ]; then
		curl -fL --retry 2 -o "$DIR/image.qcow2.xz.tmp" "$URL"
		mv "$DIR/image.qcow2.xz.tmp" "$DIR/image.qcow2.xz"
	fi
	printf '%s  %s\n' "$SHA256" "$DIR/image.qcow2.xz" | sha256sum -c -
	xz -dkc "$DIR/image.qcow2.xz" > "$BASE.tmp"
	mv "$BASE.tmp" "$BASE"
	qemu-img info "$BASE"
	;;
seed)
	mkdir -p "$DIR/key" "$DIR/seed"
	[ -f "$KEY" ] || ssh-keygen -q -t ed25519 -N '' -C ws131-freebsd-guest -f "$KEY"
	printf 'instance-id: ws131-freebsd\nlocal-hostname: keiland-freebsd\n' > "$DIR/seed/meta-data"
	{
		printf '#cloud-config\nssh_pwauth: false\nusers:\n  - name: kei\n    groups: wheel\n    shell: /bin/sh\n'
		printf '    ssh_authorized_keys:\n      - %s\n' "$(cat "$KEY.pub")"
		printf 'runcmd:\n'
		printf '  - mkdir -p /root/.ssh\n  - chmod 700 /root/.ssh\n'
		printf '  - echo "%s" > /root/.ssh/authorized_keys\n' "$(cat "$KEY.pub")"
		printf '  - chmod 600 /root/.ssh/authorized_keys\n'
		printf '  - echo "PermitRootLogin prohibit-password" >> /etc/ssh/sshd_config\n'
		printf '  - sysrc sshd_enable=YES\n  - service sshd restart\n'
	} > "$DIR/seed/user-data"
	xorriso -as mkisofs -quiet -o "$DIR/seed.iso" -V cidata -J -r "$DIR/seed/user-data" "$DIR/seed/meta-data"
	;;
start)
	mkdir -p "$GUEST"
	if [ "${OVERLAY:-new}" != keep ] || [ ! -f "$GUEST/disk.qcow2" ]; then
		rm -f "$GUEST/disk.qcow2"
		qemu-img create -q -f qcow2 -F qcow2 -b "$(realpath "$BASE")" "$GUEST/disk.qcow2" 20G
	fi
	cp "$DIR/seed.iso" "$GUEST/seed.iso"
	firmware=
	[ "${FIRMWARE:-bios}" != uefi ] || firmware="-bios /usr/share/ovmf/OVMF.fd"
	# shellcheck disable=SC2086
	qemu-system-x86_64 -m 8192 -smp 4 -accel kvm -cpu host $firmware \
		-drive "file=$GUEST/disk.qcow2,format=qcow2,if=virtio" \
		-drive "file=$GUEST/seed.iso,format=raw,media=cdrom,readonly=on" \
		-device virtio-vga,id=video0 -device qemu-xhci,id=xhci \
		-device usb-tablet,bus=xhci.0 -device usb-kbd,bus=xhci.0 \
		-netdev "user,id=net0,hostfwd=tcp:127.0.0.1:$PORT-:22" -device virtio-net-pci,netdev=net0 \
		-display none -serial null -monitor none \
		-qmp "unix:$GUEST/qmp.sock,server=on,wait=off" \
		-daemonize -pidfile "$GUEST/qemu.pid"
	echo "freebsd-guest: started pid=$(cat "$GUEST/qemu.pid") port=$PORT"
	;;
wait)
	limit=${WAIT:-600}
	waited=0
	until $SSH true 2>/dev/null; do
		waited=$((waited + 10))
		[ "$waited" -lt "$limit" ] || { echo "freebsd-guest: no SSH after ${limit}s"; exit 1; }
		sleep 10
	done
	echo "freebsd-guest: SSH answers after about ${waited}s"
	;;
ssh)
	shift
	$SSH "$@"
	;;
shot)
	qmp shot "$(realpath -m "$2")"
	;;
stop)
	qmp quit || true
	sleep 3
	if [ -f "$GUEST/qemu.pid" ] && kill -0 "$(cat "$GUEST/qemu.pid")" 2>/dev/null; then
		kill "$(cat "$GUEST/qemu.pid")"
	fi
	;;
*)
	sed -n '2,16p' "$0"
	exit 2
	;;
esac
