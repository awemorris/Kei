# q577 / P8 generation 2 recovery

2026-10-02 06:32 UTC recovery checkpoint (recorded 06:33 UTC). Base/HEAD `25729c88a`.
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

## Checkpoint verification correction

Main reviewed MR04 after the checkpoint commit. `git diff --check` on the prior
working diff had omitted the then-untracked evidence. Checking the committed
change reports trailing spaces on 12 original C9 result lines. Those raw log
bytes remain unchanged; they are not generated source formatting. The full
commit whitespace check is therefore not a clean PASS. Source/Markdown checks
are reported separately, and original artifacts retain their hashes.

## Popup observation procedure

`plan/ws099/tests/p017-popup-observe.py` copies the current p076 helpers and
original stages 1–4 into each output's `observe.sh`. It enables only the existing
`--log-frames` diagnostic interface and saves complete compositor logs. The
original pixel/color expectations and first capture verdict remain unchanged.
Only after a failed first capture, it takes six subsequent pictures at half-second
intervals and retains each picture/log even if a later picture matches. Capture
and SSH overhead make wall time longer than three seconds; the retry count and
sleep intervals are fixed. It always returns the original capture status.
This is diagnosis, not a shared harness patch or a whole-p076 acceptance run.

Maximum 20 shortened popup observations run under owner PID251642 in the existing
single runtime, each bounded by a five-minute timeout. They stop at the first
original verdict failure. Full C9 supplement and observation use separate guest
runtime directories/QEMU processes but share host CPU/GPU renderer resources.
Concurrent host load may affect timing; observation success does not establish
that the historical failed picture was only late. No production switches or
source modifications have been introduced.

## 06:35 UTC popup observation result

The first shortened observation failed in 47 seconds; its loop stopped, so only
one observation was executed. [Original verdict and all six later captures](evidence/g2/popup-observation1/verdict.txt),
[full frames](evidence/g2/popup-observation1/frames.log),
[probe](evidence/g2/popup-observation1/probe.log),
[generated procedure](evidence/g2/popup-observation1/observe.sh).
Stages 1–3 pass, including the ordinary submenu. The far menu opens, but the
flipped submenu configure is MISSING. The first orange pixel check and all six
subsequent checks fail; this cannot be cleared with a pixel-only wait.

The frames show frame16 submit at 5938823000us, shown at 5941483000us: a 2660ms
completion interval. The far menu is created/configured/mapped only afterward,
and frame18 is shown at 5941873000us. The probe logs `open menu at=360,60`,
then window button releases including local460,190, then the menu configure/focus.
It contains no corresponding submenu press or open/configure. This supports a
harness interaction issued before the target menu was ready in this shortened
concurrent-load observation; it does not prove the cause of the historical
resumed16 failure, where submenu configure and map both exist. This observation
has no duplicate request-handshake changes or retries in the shared p076.

No additional observation repetitions are needed to establish that this attempt
cannot support a pixel-only repair. A future scoped diagnosis can compare a new
far-menu mapping handshake before clicking with the original procedure. The
historical mapped-but-invisible submenu still needs separate frame/capture
observation. Compositor repair and any expansion remain for main selection.

Conformance for the new Python diagnostic and generated shell: full applicable
AGENTS/Guardrail/automation and established non-C conventions reviewed. Python
syntax compile and generated `sh -n` pass; runtime produced the expected preserved
FAIL and six later images without converting failure to success. C source,
HAL, toolchain, production configuration, and shared p076 are unchanged; C
formatter/style-check and product build are inapplicable to this source change.
The existing framebuffer boot PASS remains the boot evidence. Original raw log
whitespace is excluded from the source/Markdown whitespace check, not rewritten.
