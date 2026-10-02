# q583-i01 B3 popup timeline diagnosis

Status: in-progress / scoped diagnosis. Whole ws099-p017 remains uncleared;
BUG-125 remains reproduced / tracking. No product or shared regression changes.

## Authority and scope

Agent B dispatch on 2026-10-02: current user requested N=3 work; Agent A reserved
q581–q583 for B. Canonical scope is Agent B's `plan/agents/B3/queue.md`, with
the preserved p017 snapshot at `plan/agents/B3/approved-phase.md`, SHA256
`a111d019d78868c493edd8c4f8542a7a67d85648fa2b33a1a5fda905bf5141c3`.
The snapshot preserves prior whole-Phase criteria; the Queue narrows this attempt
to two popup diagnoses, each at most five partial guest runs, 240 seconds per run,
90 minutes total. Scope start was approximately 07:13 UTC, deadline 08:43 UTC.
No 20-whole-p076/five-C9 acceptance or comprehensive repair is claimed here.

Owned source: [p017-popup-timeline.py](../tests/p017-popup-timeline.py).
Owned durable evidence: [evidence/q583](evidence/q583).
Temporary runner/runtime/artifacts: `build/b3-q583/` in the B3 worktree.
Compositor, shared p076, other WSs and Agent A's shared records are read-only.

## Prepared diagnostic

The helper copies the actual shared p076 stages 1–4 and enables the existing
`--log-frames`. Original pixel expectations, first failure and application errors
remain authoritative for the partial test. Host monotonic/UTC times bracket
pointer, SSH and capture operations. Complete application logs surround captures;
guest frame timestamps retain their own clock domain. Six later half-second
captures may explain a failure; they never replace the original failed verdict.

The second mode waits for fresh far-menu map/configure/focus counts before the
submenu click. It retains the latest mapped surface ID. Polling is six half-second
intervals with a ten-second SSH deadline; missing readiness terminates the partial
test before the second click. Whole-run deadline terminates the owned shell process
group after 240 seconds; guest/runtime retirement is a separate cleanup step.

All diagnostic SSH, Python wrappers and log snapshots add overhead. Results do
not establish the original uninstrumented failure rate, and successful readiness
cannot prove that every historical move/resize failure is repaired.

## Preparation checks

Python in-memory syntax compile: PASS. Original and handshake generated runners
both pass `sh -n`. Host negative checks preserve the first capture FAIL even when
all six later captures match; stale/timeout readiness emits only the opening click,
while a successful readiness reply permits the second click.
[Host report](evidence/q583/host-validation.txt). These use stubbed transport/capture
replies and do not prove actual guest mapping or rendering. Poll conditions were
also manually reviewed for each count advancing beyond its pre-click baseline.

Full applicable AGENTS/Guardrail/automation and C standard were loaded. There is
no C change; new Python and generated shell follow existing diagnostic conventions.
Source/Markdown whitespace check is scoped to owned paths. Product build, C
formatter/style-check, actual guest tests and boot are not yet performed.

## Prerequisites and resource hold

Old P8 image exists at `/home/awe/zedBSD-worktrees/p8/build/p8-q577/criteria.img`,
2,216,689,664 bytes, SHA256
`992e83f498cda2a1d506bb6ad7975b3c4592069fcb6493eca707e0d84403ac37`.
Strict renderer exists at `/home/awe/zedBSD-claude1/build/ws035-sq-venus/install`;
`libexec/virgl_render_server` SHA256 is
`745bef59c17c88975ee567c0ae8c2c689b5b06fca79171d121227d0f5f5f84c2`.
Both renderer environment variables must be explicit. The old image is a
diagnostic asset with q577 provenance, not a fresh full-current-HEAD build.

B3 HEAD/base: `ea55c9739d7c5e179297271c11add7ca4b1252b8`.
Relevant popup/window implementation files match old source `8fb18105`;
popup-probe main.c is an exact rename. Later build/backend/header differences
prevent asserting full-image current source equivalence.

At preparation, B1 GTK4 QEMU PID 283995 is active. Agent B main instructed B3 to
hold Venus startup until B1's stop is confirmed. No guest, image copy, shared
renderer change or secret-key read/output has occurred. Host helper preparation
continues during that resource hold. p128 and C2 remain separate observations.
