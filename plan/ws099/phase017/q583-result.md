# q583-i01 terminal result / B3

Scoped item outcome: **cleared — finite diagnosis completed with non-reproduction**.
Whole ws099-p017: **uncleared**. BUG-125: **reproduced / tracking**, unchanged.
No product or shared p076 repair was made; the historical failures remain valid.
Agent B main reviews this scoped result and Agent A owns shared projections.

## Observed outcomes

| Mode | Partial guest runs | First flipped pixel | Map before flipped capture | Shown frame before/after capture |
| --- | --- | --- | --- | --- |
| Original stages 1–4 with timeline | 5/5 PASS | 5/5 orange `e2a04a` at 540,518 | 5/5 surface35 at 440,473 | Same frame on both sides in all 5 |
| Fresh far-menu map/configure/focus handshake | 5/5 PASS | 5/5 orange `e2a04a` at 540,518 | 5/5 surface35 at 440,473 | Same frame on both sides in all 5 |

All ten runs retain first-capture verdicts, complete compositor/probe application
logs, host input/SSH/capture brackets, frame submit/shown times and four original
PNGs. Each generated runner passed `sh -n`. Each run completed in 30.228–50.094
seconds, below its 240-second deadline. All 80 capture-adjacent log snapshots
completed successfully. No later-capture retries were needed in the guest.
The five handshake runs recorded `P017_FAR_READY=1` and a fresh map surface29 at
800,353 before sending the submenu click.

[Raw run results](evidence/q583/results.json),
[per-run classification](evidence/q583/classification.json),
[representative original verdict](evidence/q583/original-1/verdict.txt),
[representative handshake verdict](evidence/q583/handshake-5/verdict.txt),
[representative PNG](evidence/q583/handshake-5/flipped.png).
The orange submenu is visible at 440,473 in the representative PNG; manual
inspection agrees with the automated pixel result. All 40 original PNGs are
retained unmodified alongside the ten scripts, timelines and snapshot sets.

Host flipped-capture brackets measure 215.288–265.689 ms. These host values do
not share an origin with guest frame times. Original run3 has a 3183-ms interval
from submit to shown **for initial frame1 with zero windows**; its later popup
steps still pass. Other measured submit→shown intervals are at most 11 ms.
This initial-frame delay does not reproduce the prior far-menu frame16 delay.

## Classification and limits

1. **Mapped-but-invisible submenu:** unreproduced in the five original
   instrumented runs. Every current flipped capture has a mapped submenu and
   completed frame before capture, with matching orange pixels. Original q577
   resumed16 had configure/map but lacked frame/capture timing; that failure
   remains unresolved. Current successes supply no evidence that its cause was
   repaired or that mapping alone guarantees visibility.
2. **Second click before far-menu mapping:** unreproduced in the five original
   runs; all five bounded handshake variants also pass. Thus this attempt cannot
   demonstrate an improvement caused by readiness synchronization. The q577
   shortened concurrent-load failure retains its absent submenu configure/press
   and 2660-ms frame16 evidence. Its cause is not merged with symptom1.

The original mode copies the original actions and expectations, but Python
wrappers, diagnostic tracing and SSH log snapshots add timing overhead. Eight
snapshots per run can allow settling that the uninstrumented test lacks. All runs
were serialized after B1's guest stop; B2 guest work waited, unlike q577's
concurrent-load observation. No additional load injection or conditions were
introduced after the finite runs succeeded.

These ten runs stop after stage4. They are **not** ten whole p076 runs, not C9
runs, and not a substitute for p017's 20-whole-p076/five-C9 zero-failure criteria.
Historical C2 and p128 failures were not investigated. No physical-machine
acceptance or comprehensive window fix is claimed.

## Environment, commands and cleanup

2026-10-02; base `ea55c9739d7c5e179297271c11add7ca4b1252b8`;
prepared helper commit `2d33d3e7`, integrated by Agent B at `c2a8ba9f`.
QEMU 10.0.11, Python 3.13.5; zedBSD Venus, host Lavapipe through the existing
strict-queue renderer. Image and renderer provenance/hashes are recorded in
[preparation](q583-diagnosis.md). The image and existing harness identity were
copied privately into B3's ignored build/guest areas. No credentials are evidence.

Commands from `/home/awe/zedBSD-worktrees/b3`:

```sh
OUTPUT=build/b3-q583/boot BOOT_TIMEOUT=180 bash plan/tools/boot-test.sh build/b3-q583/criteria.img
GUEST_RUNTIME=/home/awe/zedBSD-worktrees/b3/build/b3-q583/runtime VENUS_RENDERER=/home/awe/zedBSD-claude1/build/ws035-sq-venus/install RENDER_SERVER_EXEC_PATH=/home/awe/zedBSD-claude1/build/ws035-sq-venus/install/libexec/virgl_render_server sh plan/ws035/tests/zdesktop-guest.sh start build/b3-q583/criteria.img
GUEST_RUNTIME=/home/awe/zedBSD-worktrees/b3/build/b3-q583/runtime python3 plan/tools/guest/guest.py wait --timeout 180
GUEST_RUNTIME=/home/awe/zedBSD-worktrees/b3/build/b3-q583/runtime python3 plan/ws099/tests/p017-popup-timeline.py build/b3-q583/original-1 --mode original
GUEST_RUNTIME=/home/awe/zedBSD-worktrees/b3/build/b3-q583/runtime python3 plan/ws099/tests/p017-popup-timeline.py build/b3-q583/handshake-1 --mode handshake
GUEST_RUNTIME=/home/awe/zedBSD-worktrees/b3/build/b3-q583/runtime python3 plan/tools/guest/guest.py stop
```

The two diagnostic commands each ran for suffixes1–5, once each. Boot-test PASS;
[login PNG](evidence/q583/boot-login.png) was inspected for the login prompt.
Console/serial logs were not read or used for verdicts. Owned QEMU297933 and its
renderer were stopped by 07:33:58 UTC; no owned QEMU/renderer/diagnostic process
or runtime/session.json remains. [Cleanup receipt](evidence/q583/cleanup.txt).
Agent B main was notified that B2 can use QEMU resources.

195 durable raw assets are protected by [SHA256SUMS](evidence/q583/SHA256SUMS).
Ignored guest disk/runtime and original build copies remain available. No shared
build, toolchain, HAL API, compositor, p076 harness or other WS was modified.
GitHub publication remains deferred and no push occurred.

## Resume

Do not run further repetitions under q583. Select a new exact-scope Queue only
when there is a concrete new discriminating condition, such as reproducing the
q577 concurrent load with coordinated resources, or preserving first capture
timing with lower instrumentation overhead. Shared regression changes require
their own verified cause and acceptance scope. Compositor repair requires a
separate authorized Phase. Existing reproduced evidence and whole-Phase criteria
remain unchanged.
