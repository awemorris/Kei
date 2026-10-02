# q577 / P8 generation 2 recovery

2026-10-02 06:40 UTC checkpoint. Base/HEAD `25729c88a`.
q577-i01 and ws099-p017 remain in-progress; deadline 07:50:43 UTC is unchanged.
Generation 1 stopped at the model usage limit. Main supplied recovery and restart.
Shared Queue/WS/Bug/cache remain main-owned. Product source and toolchain are read-only.

## Recovered outcomes

[Original single results](evidence/g2/single-original-results.txt) contain 14 PASS.
Original run 15 was interrupted and has no result. Main's [resumed results](evidence/g2/single-resumed-results.txt)
contain run 15 PASS and run 16 FAIL. Runs 17–20 did not run.
Run 16 fails only the flipped submenu pixel: expected orange `e2a04a` at 540,518,
observed dark window `2b3444`. [Full verdict](evidence/g2/single-resumed16.txt),
[probe log](evidence/g2/single-resumed16-probe.log),
[compositor log](evidence/g2/single-resumed16-zdesktop.log),
[original PNG](evidence/g2/single-resumed16-flipped.png).
Submenu configure x=-360,y=120 and map x=440,y=473 occur. These events establish
configuration/mapping but do not prove that the captured frame contains the submenu.
The move and resize request handshakes, final 550/200/800 geometry, settled anchors,
and their pixels all pass. The popup symptom is unresolved; drawing delay is only
a hypothesis. No popup fix, retry, or criterion change has been applied.

[Original C9 run 1](evidence/g2/c9-original1-results.txt) passes all 10 tests.
Main's inherited [resume-c9.sh](evidence/g2/inherited-resume-c9.sh) did not export
VENUS_RENDERER. At recovery, /proc/243882/environ and /proc/243884/environ have no
VENUS_RENDERER. zdesktop-guest.sh defaults to the worktree's
`build/ws035-sq-venus/install`, which does not exist. Main explicitly acknowledged
that its takeover script omitted the renderer environment. The original single
QEMU PID157033 instead has VENUS_RENDERER pointing to main's existing renderer
`/home/awe/zedBSD-claude1/build/ws035-sq-venus/install`; its renderer child PID157159
uses that executable. No renderer or shared build changes were made.

The invalid-environment [C9 run 2 results](evidence/g2/c9-invalid2-results.txt)
contain p052 FAIL477s and p053 FAIL121s. [p052](evidence/g2/c9-invalid2-p052.log)
reports `ZWL VULKAN_ERROR operation=device result=-3` and black pixels;
[p053](evidence/g2/c9-invalid2-p053.log) also reports black pixels.
[Original black PNG](evidence/g2/c9-invalid2-black.png) is retained. These failures
are classified as an invalid-environment attempt, not ignored or converted to PASS.
The remaining tests of that run were interrupted before verdicts.

## Bounded recovery and ownership

Stopped inherited process tree PID243882/243884/250078 with SIGTERM, then stopped
only its runtime `build/ws035-sq-run` with zdesktop-guest.sh. Kept original scripts
and all original artifacts. Single runtime `build/p8-q577/runtime` and QEMU157033
remain available; no duplicate single loop was started.

A fresh full C9 is running at `build/p8-q577/g2-c9-1` with explicit VENUS_RENDERER
and RENDER_SERVER_EXEC_PATH from the existing main renderer. Current criteria PID250478
(timeout owner250477) uses its own worktree runtime. The background wrapper ended
when its command session closed; its independent timeout/criteria children remain
alive. No subsequent C9 repetition will start. Main directed at most one bounded
full C9 supplement or diagnosis if the required five cannot fit the deadline.
This supplementary result cannot reduce or replace the original 20/5 zero-failure
acceptance criteria. The phase cannot clear on current evidence.

Next: finite popup observation if feasible, finish the bounded C9 supplement, review
all harness changes, save final evidence and any unmet criteria for main. GitHub
publication remains deferred; no push, product build, HAL/toolchain changes, or
serial/console-log verdicts.
