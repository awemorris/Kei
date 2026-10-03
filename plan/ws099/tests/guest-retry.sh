# ws099-p023: the SSH retry the zdesktop tests share (sourced, not run).
# guest_retry SECONDS COMMAND runs a guest command (plan/tools/guest/guest.py run) within a host deadline per attempt;
# when ssh itself fails (status 255: the guest's sshd did not answer in time, BUG-135), it tries again, three
# attempts in all.  The command's own status and output are kept.  Each retry is printed on stderr, so a harness
# failure is told apart from the compositor's (2026-10-02 Q1: an SSH failure is the harness's, not a FAIL of the test).
#   . plan/ws099/tests/guest-retry.sh
#   guest() { guest_retry 90 "$1" </dev/null; }
# Copyright (C) 2026 Awe Morris; SPDX-License-Identifier: Zlib
guest_retry() {
	retry_attempt=1
	while :; do
		retry_reply=$(timeout "$1" python3 plan/tools/guest/guest.py run "$2" 2>&1)
		retry_status=$?
		[ "$retry_status" -ne 255 ] && break
		[ "$retry_attempt" -ge 3 ] && break
		echo "harness: ssh failed, attempt $retry_attempt: $(printf '%s\n' "$retry_reply" | tail -1)" >&2
		retry_attempt=$((retry_attempt + 1))
	done
	[ "$retry_status" -eq 255 ] && echo "harness: ssh failed, attempt $retry_attempt (last): $(printf '%s\n' "$retry_reply" | tail -1)" >&2
	[ -n "$retry_reply" ] && printf '%s\n' "$retry_reply"
	return "$retry_status"
}
