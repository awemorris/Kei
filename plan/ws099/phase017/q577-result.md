# q577-i01 terminal result / P8 generation 2

Outcome: **uncleared**. Phase: ws099-p017 / normal / uncleared.
2026-10-02 06:48 UTC. User directed all ongoing agents to wrap up and stop.
Deadline07:50:43 UTC was not extended; no new Queue started. Main owns Queue/WS/Bug
projection and publication. P8 owns only the submitted Phase/test/evidence drafts.

## Outcomes and retained limits

| Run | Actual outcome | Acceptance implication |
| --- | --- | --- |
| Final shared p076 original single loop | 14 PASS; original run15 interrupted with no verdict | 20 runs not completed |
| Main resumed single loop | run15 PASS; run16 FAIL;17–20 unstarted | Total16 completed whole tests:15 PASS/1 FAIL; zero-failure criterion unmet |
| Original full C9 run1 | 10/10 PASS | One completed full C9, not five |
| Main recovered C9 run2 without renderer environment | p052 FAIL477s / p053 FAIL121s; rest interrupted | Invalid-environment failure retained; no product cause claim |
| Explicit-renderer C9 supplement | p052 PASS34s / p053 PASS72s / p072 PASS48s / p076 PASS66s / p126 PASS106s / p128 FAIL45s | Partial run:5 PASS/1 FAIL; not a completed C9 |
| Supplement p134/p137/p138/cursor-owner | p134 guest start had begun; no verdict; last3 unstarted | All4 remain unverified in this supplement |
| Shortened framed popup observation | 1 FAIL47s; loop stops | Diagnostic stages1–4 only; not counted as whole p076 |

The shared accepted-request synchronization patch is retained at
`fb8732b939294e7e98f4362150298dbe929f7723` (main integration recorded separately).
It prevents old counts and button release from falsely satisfying an accepted move
or resize. Current whole-p076 trials preserve the expected550/200/800 sizes and
anchors. Run16 and the corrected C9 p076 satisfy all move/resize/settled/pixel
checks. These partial results support that harness change but cannot establish
that every historical window failure is repaired.

Two popup symptoms remain distinct:

1. Original resumed16 configures and maps the flipped submenu, but its first
   PNG has dark window instead of orange. No frames were logged there; drawing
   or capture delay remains unproven. The main inherited raw log/PNG are preserved.
2. The shortened concurrent-load observation has no flipped submenu configure
   or corresponding press. The far menu maps after the next click. Frame16
   completion takes2660ms. The original and six later PNGs stay dark. This is
   an interaction-before-ready diagnosis, not a pixel-only delay or proof of
   the resumed16 cause. No shared popup fix was applied.

Main acknowledged its takeover script omitted VENUS_RENDERER, while the
worktree-default renderer did not exist. The resulting p052/p053 failures are
retained. The supplement explicitly uses the existing main strict-queue renderer;
p052/p053 then pass. Its p128 failure is a separate observation:
`ZWL HANDOFF go=1` exists but `ZWL GLASS launch surface=` is MISSING; no PNG was
produced by that early failure. Its cause was not investigated after the stop
instruction. Do not silently merge it into BUG-125 or claim a new product defect.

C2 q538's geometry failures remain unresolved. BUG-125 must remain tracking;
there is no whole-Phase clearance or verified comprehensive repair.

## Evidence and conformance

[Recovery/classification](recovery.md), [source review](conformance.md),
[partial supplement results](evidence/g2/c9-supplement/results.txt),
[whole p076 supplement](evidence/g2/c9-supplement/p076.log),
[p128 failure](evidence/g2/c9-supplement/p128.log),
[cleanup receipt](evidence/g2/cleanup.txt), [artifact hashes](evidence/g2/SHA256SUMS).
Original failures and scripts are retained. The supplement's six raw test logs,
p076 logs/allPNG, and p072 pictures are committed. Other bulk success artifacts
remain in the preserved worktree under `build/p8-q577/g2-c9-1`.

For p072, pixel/log automation passes. Manual review of wiseview/dragging/desk2/
desk1 PNG confirms the pale minimized blue tile, drag toward desktop2, red-only
window on desktop2, and both windows on desktop1. It does not substitute for
physical-machine acceptance. [Existing boot/login](evidence/boot-login.png)
remains the boot-test evidence; no additional boot/source build was needed.

Full applicable source review, shell syntax, Python compile and scoped source/
Markdown whitespace pass. Immutable raw C9 result whitespace is preserved and
excluded from that scoped PASS. No compositor/C/HAL/toolchain/Noct/LLVM changes,
production switches, shared build mutations, push, or serial/console-log verdict.
QEMU10.0.11 / Python3.13.5 / dash0.5.12-12. Full WS099 conformance is outside q577.

## Stop, assets and restart

Stopped single QEMU157033/renderer157159 at06:41UTC. At06:47UTC stopped timeout/
criteria250477/250478 and descendant, then both dedicated runtime stop commands.
Read-back at06:48UTC finds no owned QEMU/renderer/criteria/diagnostic process, no
runtime session.json, and no active Unix listener under this worktree build root.
There is no live owner left for Agent B to inherit. Guest disks and all artifacts
are preserved, including main takeover scripts; no ignored asset was deleted.

Read-only restart assets:

- Worktree `/home/awe/zedBSD-worktrees/p8`, branch`codex/p8`; final submitted SHA in MR06.
- Image `/home/awe/zedBSD-worktrees/p8/build/p8-q577/criteria.img`, SHA256
  `992e83f498cda2a1d506bb6ad7975b3c4592069fcb6493eca707e0d84403ac37`.
- Renderer `/home/awe/zedBSD-claude1/build/ws035-sq-venus/install` is existing and
  read-only. Explicitly export VENUS_RENDERER and RENDER_SERVER_EXEC_PATH for a
  future authorized owned runtime. Do not rely on an absent clone-local default.
- Original single/C9/observation/supplement artifacts remain under
  `/home/awe/zedBSD-worktrees/p8/build/p8-q577/`; credentials are not in committed evidence.

Resume requires main/B to reconcile terminal q577 and select a finite approved
new attempt with the exact revised criteria; q577 is not resumed automatically.
Proposed diagnosis scope: independently observe mapped-but-invisible flipped
submenu with per-frame and capture times, compare a newly mapped far-menu
handshake before the second click with original input, preserve the original
negative/count/geometry conditions, and prove any synchronization change before
modifying the shared regression. A compositor change needs a separate selected
Phase, since this one grants read-only compositor access. p128 and C2 require
classification/appropriate separate scope. None of these proposals grants execution.
