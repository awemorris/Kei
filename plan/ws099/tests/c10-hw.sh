#!/bin/sh
# Exercises C10 on the 5330's existing i915 passthrough fixture.
# Usage: c10-hw.sh IMAGE OUTDIR [MINUTES]; default MINUTES is 60.
# The source image is copied by H4. Only this run's exact owner and QEMU PID
# may be stopped. Session evidence is read from its disk, never a console.
# Copyright (C) 2026 Awe Morris; SPDX-License-Identifier: Zlib
set -u
cd "$(dirname -- "$0")/../../.." || exit 1

# Rejects incomplete arguments before creating or acquiring any resource.
if [ "$#" -lt 2 ] || [ "$#" -gt 3 ]; then
	echo "usage: $0 IMAGE OUTDIR [MINUTES]" >&2
	exit 2
fi

image=$1
out=$2
minutes=${3:-60}
host=${I915_HOST:-solaris10-man}
h4=plan/ws075/tests/hdmi-h4-hw.sh
owner=/tmp/i915-h4-owner
case "$minutes" in
''|*[!0-9]*) echo "c10-hw: MINUTES must be a positive integer" >&2; exit 2 ;;
esac

# Refuses a reused evidence directory and a missing source image.
if [ "$minutes" -eq 0 ] || [ ! -f "$image" ] || [ -e "$out" ]; then
	echo "c10-hw: use a present IMAGE, positive MINUTES and new OUTDIR" >&2
	exit 2
fi
mkdir -p "$out" || exit 1
out=$(cd "$out" && pwd) || exit 1

# Sends one bounded interaction to the owner-checked H4 controller.
ctl() {
	timeout 120 "$h4" ctl "$@"
}

# Finds the QEMU using this fixture's disk, excluding the survey shell.
fixture_pid() {
	ssh -o BatchMode=yes -o ConnectTimeout=8 "$host" "pgrep -f '^qemu-system-x86_64.*file=/home/awe/bigbang/h4/guest.img' || true"
}

# Runs in a subshell so its exit status survives the output's tee pipeline.
run() (
	set -e
	started=0
	qemu_pid=

	# Returns only a fixture owned by this exact OUTDIR and original QEMU.
	cleanup() {
		status=$?
		trap - 0 HUP INT TERM
		if [ "$started" -eq 1 ]; then
			current_owner=$(cat "$owner" 2>/dev/null || true)
			if [ "$current_owner" = "$out" ]; then
				current_pid=$(fixture_pid) || current_pid=unknown
				if [ -z "$current_pid" ] || [ "$current_pid" = "$qemu_pid" ]; then
					"$h4" stop "$out" > "$out/cleanup.out" 2>&1 || status=1
				else
					echo "c10-hw: changed QEMU PID; cleanup refused, owner retained"
					status=1
				fi
			elif [ -f "$out/.running" ] && [ ! -f "$out/.locked" ]; then
				# Cancels only this run's unacquired lock wait.
				rm -f "$out/.running"
			else
				echo "c10-hw: changed owner; cleanup refused"
				status=1
			fi
		fi
		if [ "$status" -ne 0 ]; then
			echo "c10-hw: FAIL"
		fi
		echo "$status" > "$out/exit.status"
		exit "$status"
	}
	trap cleanup 0
	trap 'exit 130' HUP INT TERM

	# Preserves the GPU's current binding and refuses an existing remote VM.
	driver=$(ssh -o BatchMode=yes -o ConnectTimeout=8 "$host" 'basename "$(readlink /sys/bus/pci/devices/0000:00:02.0/driver)"')
	remote_vm=$(ssh "$host" "pgrep -f '^qemu-system' || true")
	if [ "$driver" != vfio-pci ] || [ -n "$remote_vm" ] || [ -e "$owner" ]; then
		echo "c10-hw: fixture unavailable; existing owner/VM/binding preserved"
		exit 1
	fi
	flock -n /tmp/i915-hw.lock true || { echo "c10-hw: hardware lock busy"; exit 1; }

	# H4's vfio request is idempotent for the already verified binding.
	started=1
	H4_MINUTES=$((minutes + 20)) "$h4" start "$image" "$out"
	sleep 5
	qemu_pid=$(fixture_pid)
	case "$qemu_pid" in
	''|*[!0-9]*) echo "c10-hw: expected one fixture QEMU"; exit 1 ;;
	esac
	echo "$qemu_pid" > "$out/qemu.pid"
	sleep 70
	ctl shot desktop > "$out/desktop-shot.out"

	# Opens ten applications using the demonstration's established tile order.
	for tile in files:600:386 notes:743:386 settings:887:386 terminal:1031:386 pdf:1175:386 images:1319:386 browser:600:538 mview:743:538 gears:887:538 xterm:1031:538; do
		set -- $(echo "$tile" | tr : ' ')
		ctl pointer move 22 16 sleep 100 down up sleep 1500 move "$2" "$3" sleep 150 down up sleep 8000 > /dev/null
	done
	ctl shot opened > "$out/opened-shot.out"

	# Measures actual wall time across menu transitions and reversible drags.
	n=0
	start=$(date +%s)
	end=$((start + minutes * 60))
	now=$start
	while [ "$now" -lt "$end" ]; do
		ctl hmp 'sendkey meta_l-tab' > /dev/null
		sleep 2
		ctl hmp 'sendkey esc' > /dev/null
		sleep 2
		ctl pointer move 22 16 sleep 100 down up sleep 2000 > /dev/null
		ctl hmp 'sendkey esc' > /dev/null
		sleep 2
		ctl pointer move 960 300 sleep 100 down sleep 100 move 1060 340 sleep 300 up sleep 500 > /dev/null
		ctl pointer move 1060 340 sleep 100 down sleep 100 move 960 300 sleep 300 up sleep 500 > /dev/null
		n=$((n + 1))
		if [ $((n % 10)) -eq 1 ]; then
			# Opens and closes an extra Terminal while preserving the ten apps.
			ctl pointer move 22 16 sleep 100 down up sleep 1500 move 1031 386 sleep 150 down up sleep 6000 > /dev/null
			ctl shot "window-open-$n" > "$out/window-open-$n-shot.out"
			ctl keys "'exit\\n'" > /dev/null
			sleep 3
			ctl shot "window-closed-$n" > "$out/window-closed-$n-shot.out"
		fi
		if [ $((n % 20)) -eq 0 ]; then
			ctl shot "round-$n" > "$out/round-$n-shot.out"
		fi
		now=$(date +%s)
		printf 'rounds=%s elapsed_seconds=%s qemu_pid=%s\n' "$n" "$((now - start))" "$qemu_pid" > "$out/checkpoint.new"
		mv "$out/checkpoint.new" "$out/checkpoint.txt"
		if [ $((n % 5)) -eq 0 ]; then
			echo "C10-HW CHECKPOINT $(cat "$out/checkpoint.txt")"
		fi
	done
	elapsed=$((now - start))

	# Saves a fresh session receipt through the real Terminal and captures it.
	ctl pointer move 22 16 sleep 100 down up sleep 1500 move 1031 386 sleep 150 down up sleep 6000 > /dev/null
	ctl keys "'cp /run/user/1000/session.log /home/kei/c10-hw.log; echo C10_HW_SAVED_$start >> /home/kei/c10-hw.log; sync\\n'" > /dev/null
	sleep 4
	ctl shot saved > "$out/saved-shot.out"
	ctl quit > /dev/null
	sleep 3
	scp -q plan/ws031/tests/ufs-cat.py tools/build/check-ufs-image.py "$host:bigbang/"
	ssh "$host" 'python3 bigbang/ufs-cat.py bigbang/h4/guest.img /home/kei/c10-hw.log' > "$out/session.log"
	ssh "$host" 'python3 bigbang/ufs-cat.py bigbang/h4/guest.img /var/log/sessiond.log' > "$out/sessiond.log"
	"$h4" stop "$out" > "$out/cleanup.out" 2>&1
	started=0

	# Counts application/session faults while refusing missing or stale evidence.
	errors=$(grep -Ec 'ZWL ERROR|ZWL FAILED' "$out/session.log" || true)
	restarts=$(grep -Ec 'SESSIOND GREETER (retry|failed)' "$out/sessiond.log" || true)
	echo "C10-HW RESULT rounds=$n minutes=$minutes elapsed_seconds=$elapsed errors=$errors restarts=$restarts"
	if [ "$errors" -ne 0 ] || [ "$restarts" -ne 0 ] || [ ! -s "$out/shots/saved-live.png" ]; then
		exit 1
	fi
	grep -q "^C10_HW_SAVED_$start$" "$out/session.log" || exit 1
	grep -q '^===== /var/log/sessiond.log$' "$out/sessiond.log" || exit 1
	echo "c10-hw: PASS (inspect saved-live.png for the live Terminal)"
)

# Records both successful interactions and early refusal diagnostics.
run 2>&1 | tee "$out/c10-hw.out"
status=$(cat "$out/exit.status" 2>/dev/null || echo 1)
exit "$status"
