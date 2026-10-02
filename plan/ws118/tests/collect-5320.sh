#!/bin/sh
# ws118-p001: collects a remote-log image's logs over SSH.
# Copyright (C) 2026 Awe Morris; SPDX-License-Identifier: Zlib
#
#   plan/ws118/tests/collect-5320.sh HOST OUTDIR
#
# Logs in as root with plan/tmp/guest/id_ed25519 (the key the remote-log
# images let in) and writes one file per item to OUTDIR, with the command's
# exit status in OUTDIR/index.txt.  An item that fails still leaves its file
# (with what the command printed) and the next item is tried.  SSH_PORT
# selects another port (QEMU's forwarded one); the host key is not checked
# (each image makes its own at its first boot) and is not remembered.
#
# There is no i915 diagnostic node in zedBSD yet: what the i915 reports is in
# the kernel's message buffer (dmesg), hw.gpu.attaching (in sysctl -a) and the
# device nodes under /dev, and those are what is collected.
set -u
cd "$(dirname -- "$0")/../../.."
host=${1:?host}
out=${2:?outdir}
port=${SSH_PORT:-22}
key=plan/tmp/guest/id_ed25519
mkdir -p "$out"
: > "$out/index.txt"

# One item: NAME then the remote command line.
item() {
	name=$1
	shift
	timeout 60 ssh -n -i "$key" -p "$port" \
		-o BatchMode=yes -o ConnectTimeout=10 \
		-o StrictHostKeyChecking=no -o UserKnownHostsFile=/dev/null \
		-o LogLevel=ERROR \
		"root@$host" "$*" > "$out/$name.txt" 2>&1
	status=$?
	echo "$name exit=$status: $*" >> "$out/index.txt"
}

item uname 'uname -a'
item dmesg 'dmesg'
item lspci 'lspci -Dkv'
item lsusb 'lsusb -tv'
item sysctl 'sysctl -a'
item ifconfig 'ifconfig -a'
item net-show 'net show'
item route 'route -n show'
item resolv 'cat /etc/resolv.conf'
item wifi 'net wifi list'
item dev 'ls -l /dev'
item mount 'mount'
item messages 'cat /var/log/messages'
item sessiond 'cat /var/log/sessiond.log /var/log/greeter.log 2>&1'
item session 'cat /run/user/*/session.log 2>&1'
item services 'service list'
item ps 'ps ax 2>&1 || ps 2>&1'
cat "$out/index.txt"
