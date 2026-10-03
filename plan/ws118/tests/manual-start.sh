#!/bin/sh
# ws118-p005: starts a variant D machine's i915 over SSH and collects what it logged.
# Copyright (C) 2026 Awe Morris; SPDX-License-Identifier: Zlib
#
#   plan/ws118/tests/manual-start.sh HOST OUTDIR [GREETER_SECONDS]
#
# Variant D (build-remote-log-image.sh D) boots with i915.start=manual
# i915.debug=display: the i915 attaches but is held, so the machine comes up
# on the firmware's framebuffer with SSH.  This script, as root over SSH:
#
#   1. saves the kernel log before the start (OUTDIR/dmesg-before.txt) and
#      what hw.gpu.start and hw.gpu.attaching say;
#   2. asks for the start with `sysctl hw.gpu.start=1` (OUTDIR/start.txt with
#      its exit status; on a machine without a held device it fails with
#      "Operation not supported by device"/ENODEV and nothing else changes);
#   3. waits up to 120 s for the node ("i915: registered native GPU node")
#      or a failed start ("start stopped at"), saving the kernel log every 2 s, so
#      a hang still leaves the latest copy (OUTDIR/dmesg-start.txt);
#   4. unless GREETER_SECONDS is 0 (default 60), starts the graphical login
#      (`service start greeter`), which claims the display and runs the
#      panel's modeset, and saves the kernel log every 2 s for that long
#      (OUTDIR/dmesg-greeter.txt);
#   5. collects everything else with collect-5320.sh into OUTDIR/collect.
#
# SSH_PORT selects another port (QEMU's forwarded one).  The machine's own
# screen is not needed; a photograph of it after step 4 helps.
set -u
cd "$(dirname -- "$0")/../../.."
host=${1:?host}
out=${2:?outdir}
greeter_seconds=${3:-60}
port=${SSH_PORT:-22}
key=plan/tmp/guest/id_ed25519
mkdir -p "$out"

# Runs one remote command line as root; its output goes to stdout.
remote() {
	timeout "${REMOTE_TIMEOUT:-60}" ssh -n -i "$key" -p "$port" \
		-o BatchMode=yes -o ConnectTimeout=10 \
		-o StrictHostKeyChecking=no -o UserKnownHostsFile=/dev/null \
		-o LogLevel=ERROR \
		"root@$host" "$*"
}

# Saves the kernel log to FILE every 2 s for SECONDS, or until it shows PATTERN.
follow() {
	file=$1
	seconds=$2
	pattern=$3
	started=$(date +%s)
	while [ $(( $(date +%s) - started )) -lt "$seconds" ]; do
		remote dmesg > "$file.new" 2>&1 && mv "$file.new" "$file"
		if [ -n "$pattern" ] && grep -Eq "$pattern" "$file" 2>/dev/null; then
			break
		fi
		sleep 2
	done
	rm -f "$file.new"
	echo "manual-start: $(basename "$file") after $(( $(date +%s) - started )) s"
}

# 1. The state before the start.
remote dmesg > "$out/dmesg-before.txt" 2>&1
remote 'sysctl hw.gpu.start; sysctl hw.gpu.attaching; ls -l /dev/gpu* 2>&1' > "$out/before.txt" 2>&1
cat "$out/before.txt"

# 2. The start.
remote 'sysctl hw.gpu.start=1; echo "exit=$?"' > "$out/start.txt" 2>&1
cat "$out/start.txt"

# 3. The start worker's end.
follow "$out/dmesg-start.txt" 120 'i915: registered native GPU node|i915: device 8086:[0-9a-f]+ start stopped at|hw.gpu.start: no device'
remote 'sysctl hw.gpu.start; sysctl hw.gpu.attaching; ls -l /dev/gpu* 2>&1' > "$out/after-start.txt" 2>&1
cat "$out/after-start.txt"

# 4. The graphical login, which runs the panel's modeset.
if [ "$greeter_seconds" != 0 ]; then
	remote 'service start greeter; echo "exit=$?"' > "$out/greeter.txt" 2>&1
	cat "$out/greeter.txt"
	follow "$out/dmesg-greeter.txt" "$greeter_seconds" ''
fi

# 5. Everything else.
SSH_PORT=$port plan/ws118/tests/collect-5320.sh "$host" "$out/collect" > /dev/null
grep -c . "$out/collect/index.txt" | sed 's/^/manual-start: collected items: /'
