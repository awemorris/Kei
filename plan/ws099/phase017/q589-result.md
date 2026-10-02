# q589-i01 terminal result / user-directed B3 wrap-up

Queue: **finished**. Attempt: **uncleared**. Whole ws099-p017: **uncleared**.
BUG-125: **reproduced / tracking**, unchanged. Event `b3-q589-user-wrap-20261002`.
2026-10-02 08:45 UTC. B3 received the current user's instruction via Agent B main:
「B3には作業状況を保存してもらい、unclearedで記録してもらい、再開できるようにした上で、終了してもらってください。」

This ends the finite attempt with its prepared output and unmet guest criteria.
It is not a reproduced new failure, invalidation of q583, cancellation of p017,
or a verified BUG-125 repair. Agent B main owns lane reconciliation; Agent A owns
shared Queue/history/Bug/cache/GitHub projections. No next Queue was started.

## Saved outcome and unexecuted work

The [low-overhead helper](../tests/p017-popup-low-overhead.py) copies original p076
stages1–4. Shell builtins timestamp input/capture with `/proc/uptime`, approximately
10-ms precision. q583's eight pre/post-capture log snapshots become four
post-verdict snapshots; the original pointer/check arguments and first verdict
remain unchanged. Existing `--log-frames` is preserved, with no concurrent load
or readiness variant. Each partial run has a240-second bound and six later
captures can explain but never clear first failure.

Python syntax and generated `sh -n` passed. The real generated shell check
wrapper's host negative preserved original FAIL despite six later PASS and
retained expected color/coordinates; no log retrieval preceded the first verdict.
[Raw host evidence](evidence/q589/host-validation.txt). Capture/transport were
stubbed in that host check. Full manual applicable-rule review and staged
source/Markdown whitespace passed. No additional host repetition was run during
the resource hold or wrap-up.

Prepared helper commit `0d93e7270ecf6f961b9a72f56f9024da366b49f8` was integrated by
Agent B at `42b08dab`. Timebox clarification commit
`a1c732016d52c9247dfcc77809bfb9e9d1413205` was integrated at `09e0cd83`.
Worktree `/home/awe/zedBSD-worktrees/b3`, branch `codex/b3-bug125`;
base/main checkpoint `3b2d2924`. q577/q583 original evidence remains unchanged.

**Not performed in q589:** boot-test, guest start/SSH wait, all five actual popup
observations, guest frame/input classification, first actual PNGs, product build,
whole-p076/C9 or physical checks. Thus both actual popup symptoms remain untested
by this low-overhead variant. No application/compositor/shared p076 source changed.

## Resource/timebox state

Preparation began07:49:13 UTC, checkpoint07:53:30 UTC. Main accepted a conservative
five active preparation minutes and forty remaining active minutes. External
resource waiting was separated from the45-minute active-work bound by main;
the original08:34 wall projection was replaced before any guest execution.
That external hold was bounded by B1 q587's finite deadline. B1's QEMU/host CPU/GPU
priority and the absence of main's B3 guest grant prevented starting the diagnosis.
The user then requested wrap-up while the resource hold remained active.

Those five/forty-minute figures preserve the pre-execution checkpoint and do not
authorize automatic continuation. Any resumed attempt needs main's finite Queue
selection, exact scope agreement, resource grant and newly recorded active budget.
No parallel guest/load condition is authorized by this saved attempt.

## Assets and restart

[Input/source manifest](evidence/q589/restart-assets.json) records absolute paths,
sizes and SHA256 hashes. The B3-owned input image is preserved at
`/home/awe/zedBSD-worktrees/b3/build/b3-q589/criteria.img`, size2,216,689,664 bytes,
SHA256 `992e83f498cda2a1d506bb6ad7975b3c4592069fcb6493eca707e0d84403ac37`.
It is a read-only diagnostic input copy of the old q577 image, not a fresh
current-HEAD product-build assertion. Old P8 source image also remains at
`/home/awe/zedBSD-worktrees/p8/build/p8-q577/criteria.img`.

Existing strict renderer is read-only:
`/home/awe/zedBSD-claude1/build/ws035-sq-venus/install`; previously verified
`libexec/virgl_render_server` SHA256
`745bef59c17c88975ee567c0ae8c2c689b5b06fca79171d121227d0f5f5f84c2`.
QEMU10.0.11/Python3.13.5 were observed in q583. No renderer/toolchain/system
installation was changed. Current image and helper hashes were checked only for
the required handoff, without additional behavior testing.

Existing harness identity remains privately in B3's ignored `plan/tmp/guest/`,
copied earlier from P8 to match the image. Do not print or commit credentials.
The prepared shell remains at `build/b3-q589/prepare/observe.sh`; a durable exact
copy is [prepare-observe.sh](evidence/q589/prepare-observe.sh). Both use the original
shared p076 and B3 paths; fresh generated runs need separate output directories.

After a new dispatch/grant, resume in this order:

1. Reuse the worktree/context, fast-forward main's accepted checkpoint if needed,
   and read only changed instructions and the new exact Queue scope. Confirm
   B1/B2 guest cessation, image/helper/source provenance and remaining budget.
2. Run `boot-test.sh` with a new B3-owned OUTPUT, inspect/show its login PNG.
   Start only a B3-owned runtime with both explicit renderer environment variables,
   then await loopback SSH. Never use another owner's runtime or shared build.
3. Execute at most five fresh original partial runs, each bounded at240 seconds,
   preserving first verdict/PNG, post-verdict application logs, shell timeline and
   frame trace. A timed-out/infrastructure run stops for main reconciliation.
4. Classify each of the two symptoms separately or record finite non-reproduction
   and overhead/clock limits. Preserve q577 evidence and whole-p017 criteria.
   Retire owned guest/renderer and verify processes/sockets/session disappearance.
5. Save evidence/hash/result, commit WIP on owned paths and send a main MR.
   Scope expansion, parallel load or shared regression/product repair requires
   another selected Queue.

Example commands after the new authorization, from the B3 worktree:

```sh
OUTPUT=build/b3-NEW/boot BOOT_TIMEOUT=180 bash plan/tools/boot-test.sh build/b3-q589/criteria.img
GUEST_RUNTIME=/home/awe/zedBSD-worktrees/b3/build/b3-NEW/runtime VENUS_RENDERER=/home/awe/zedBSD-claude1/build/ws035-sq-venus/install RENDER_SERVER_EXEC_PATH=/home/awe/zedBSD-claude1/build/ws035-sq-venus/install/libexec/virgl_render_server sh plan/ws035/tests/zdesktop-guest.sh start build/b3-q589/criteria.img
GUEST_RUNTIME=/home/awe/zedBSD-worktrees/b3/build/b3-NEW/runtime python3 plan/tools/guest/guest.py wait --timeout 180
GUEST_RUNTIME=/home/awe/zedBSD-worktrees/b3/build/b3-NEW/runtime python3 plan/ws099/tests/p017-popup-low-overhead.py build/b3-NEW/run-1
GUEST_RUNTIME=/home/awe/zedBSD-worktrees/b3/build/b3-NEW/runtime python3 plan/tools/guest/guest.py stop
```

Replace NEW with the actual new Queue/attempt resource prefix; do not execute
this saved procedure under finished q589.

## Cleanup and delivery

[08:45:39 UTC receipt](evidence/q589/cleanup.txt): q589 guest/boot/actual observations
were never started; owned QEMU/renderer/diagnostic processes0, owned Unix socket
entries0, no runtime directory or session.json. No stop action was needed or sent
to another owner. Image/prepare assets were preserved; no shared process was stopped.
No serial/console log was read for a verdict. WIP/no push, GitHub publication
deferred. B3 ends after this terminal MR as explicitly requested by the user.
