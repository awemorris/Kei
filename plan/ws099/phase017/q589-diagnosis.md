# q589-i01 low-overhead preparation / B3

Status: in-progress, host preparation complete; guest resource grant pending.
Whole p017: uncleared. BUG-125: reproduced / tracking.
Preparation start: 2026-10-02 07:49:13 UTC; preparation checkpoint07:53:30 UTC.
Maximum45 minutes of active work; external resource hold is recorded separately.
Authorization is [q589-approved-scope.md](q589-approved-scope.md), SHA256
`16005006ee6e1868d2ad67dc1c1328f27d27d1e43f02dd012fa31ad7877464c7`.
Base/main checkpoint: `3b2d2924`; q583 integration and all original evidence retained.

Owned helper: [p017-popup-low-overhead.py](../tests/p017-popup-low-overhead.py).
Owned evidence: [evidence/q589](evidence/q589); ignored runtime `build/b3-q589/`.
The helper copies shared p076 stages1–4. Pointer/capture commands and arguments,
first pixel expectations and original negative/log checks remain unchanged.
The existing frame trace is enabled; original map/geometry is not repaired.

Shell `read` and `printf` bracket pointer/capture using `/proc/uptime` (about10-ms
resolution). No Python wrapper or SSH snapshot runs before the first pixel
verdict. Four post-verdict application-log snapshots replace q583's eight
capture-adjacent snapshots. These post-verdict snapshots still affect subsequent
steps and are documented as remaining overhead. Clock origins are separate.
At most six later captures explain first failure while retaining that failure.
Each partial run terminates its owned host children at240 seconds; guest/runtime
cleanup follows separately. Maximum five original partial runs, no handshake,
parallel guest, injected load, shared p076 or product-source change.

Python in-memory compile and generated `sh -n`: PASS.
[Host negative evidence](evidence/q589/host-validation.txt): the actual shell check
wrapper returns original FAIL after six later PASS; exact expected color/coordinates
are preserved and event order has no log retrieval before the first verdict.
Transport/capture are stubbed in this host negative, not a guest reproduction.
Manual review uses already loaded AGENTS/Guardrail/full C/automation/protocol and
the session-b continuity addition. No new C or standard exception; C formatter,
product build, C9/whole p076 and physical checks are outside this scope.

B1 q587 has QEMU priority; no guest was started. Main will grant the resource after
B1 stops. Main's 07:55 UTC follow-up separates external resource wait from active
work time and bounds the hold by q587's own deadline. Scope/run limits are unchanged.
Preparation used about five active minutes conservatively; at most40 active minutes
remain after grant. No further helper or host repetitions during the resource hold.
Preparation source/Markdown whitespace was checked on staged owned paths.
