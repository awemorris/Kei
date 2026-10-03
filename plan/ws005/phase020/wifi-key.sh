#!/bin/sh
# ws005-p020: runs `net wifi add|modify SSID [options]` in the RTL guest as root or kei, the key coming on
# standard input from a file (mode 600, outside the worktree) and never on a command line or in a log.
#   KEYFILE=/path GUEST_RUNTIME=... sh plan/ws005/phase020/wifi-key.sh root|kei add|modify SSID [--auto yes|no]
# Without KEYFILE the key is read from this script's standard input (a made-up key for the wrong-key cases).
# kei is reached by the harness key: the first use copies root's authorized_keys to kei (test guests only).
# Copyright (C) 2026 Awe Morris; SPDX-License-Identifier: Zlib
set -eu
cd "$(dirname -- "$0")/../../.."
user=${1:?usage: wifi-key.sh root|kei add|modify SSID [--auto yes|no]}
verb=${2:?verb}
ssid=${3:?ssid}
shift 3
case $user in root|kei) ;; *) echo "wifi-key: user is root or kei"; exit 2 ;; esac
case $verb in add|modify) ;; *) echo "wifi-key: verb is add or modify"; exit 2 ;; esac
runtime=${GUEST_RUNTIME:-$PWD/build/p1-rtl-run}
port=$(python3 -c 'import json,sys; print(json.load(open(sys.argv[1]))["ssh_port"])' "$runtime/session.json")
key=plan/tmp/guest/id_ed25519
ssh_guest() {
	ssh -i "$key" -p "$port" -o StrictHostKeyChecking=no -o UserKnownHostsFile=/dev/null -o LogLevel=ERROR \
	    -o ConnectTimeout=5 -o BatchMode=yes "$@"
}
if [ "$user" = kei ]; then
	ssh_guest root@127.0.0.1 -- 'test -f /home/kei/.ssh/authorized_keys || { mkdir -p /home/kei/.ssh &&
	    cp /root/.ssh/authorized_keys /home/kei/.ssh/ && chown -R kei /home/kei/.ssh &&
	    chmod 700 /home/kei/.ssh && chmod 600 /home/kei/.ssh/authorized_keys; }' </dev/null
fi
# The SSID and options are quoted for the guest's shell; the key is only on standard input.
words="net wifi $verb '$(printf %s "$ssid" | sed "s/'/'\\\\''/g")'"
for option in "$@"; do
	words="$words '$(printf %s "$option" | sed "s/'/'\\\\''/g")'"
done
if [ -n "${KEYFILE:-}" ]; then
	ssh_guest "$user@127.0.0.1" -- "$words" <"$KEYFILE"
	exit $?
fi
ssh_guest "$user@127.0.0.1" -- "$words"
