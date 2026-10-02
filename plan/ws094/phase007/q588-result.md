# q588-i01 / partial source conformance result

2026-10-02 / B2. Base `60201ab88cbf482024a70db9bcae4f2e2d771a45`.
Previous integrated checkpoint: `8beb7e3126d15d8b886803f0c0ab98d925636eb2`
(B main `71bf741a`, lane ACK `870eb679`).
Exact approval: [q588-approved-scope.md](q588-approved-scope.md), SHA256
`a1faf4833c5289290f6ae2c42170d8be80ff0ab39b3520a04f033ec8f157a6f5`.

Outcome: **partial Queue item criteria satisfied; main review/ACK pending**.
Whole WS094 p007 is **uncleared**, WS094 **incomplete**. The read-only conformance
residuals and physical p012 / full guest / C9 / final boot gates remain intact.

## Delivered changes and review

Checkpoint 1 is preserved in [q588-checkpoint1.md](q588-checkpoint1.md).
This final checkpoint completes the 56-path commit-derived
[inventory](q588-inventory.json) and [full-standard review](q588-review.md).

Additional corrections:

- Probe types precede file variables; public main precedes static implementation;
  callback prototypes before tables are limited to initializer requirements.
  Added individual constructor/listener/roundtrip/dispatch/flush/poll/clock result
  checks. Failed setup exits safely and releases locally acquired resources;
  final disconnect releases connection-owned protocol objects. Normal messages,
  requests, drawings, observation deadlines and exit conventions are preserved.
- libkeiland desktop registry/listener/roundtrip failures now release their
  registry/wrapper/private queue and refuse creation safely. A failed partial
  roundtrip no longer supplies a manager. Successful request order is unchanged.
- WS094 startup type moved ahead of variables; selection timestamp purpose
  documented; rename checks the old lstat before requesting the new one.
- Existing host layout operations are checked individually; original test
  expectations and operation order remain. Thumbnail writes/close now fail safely
  on incomplete output and release decoded pixels. GIF optional metadata status
  is checked while preserving the previous opaque fallback.

This includes deliberate changes to failure paths. It does not claim every
failure path is behavior-identical. No new feature, public API, protocol request,
test control, weakened expectation or ownership-outside source edit was added.

## Final verification

Durable evidence: [q588-evidence.json](q588-evidence.json) (log hashes/versions),
[q588-evidence/](q588-evidence/). The durable Image Viewer host log strips trailing whitespace only; the original
local log/hash remains recorded. Full local target logs remain under
`/home/awe/zedBSD-worktrees/b2/build/b2-ws094-p007/`.

| Check | Final result |
| --- | --- |
| `sh plan/ws094/tests/host-desktop.sh` | exit 0 / PASS |
| `sh plan/ws094/tests/host-thumb.sh build/b2-ws094-p007/host-thumb` | exit 0 / all picture fixtures PASS |
| `sh plan/tools/files/host-model.sh` | exit 0 / PASS |
| `sh plan/tools/imageview/run-host.sh build/b2-ws094-p007/imageview-host` | exit 0 / PASS |
| Private target make, files/wayland/imageview/libkeiland | exit 0 / warning 0 |
| Edited-range clang-format-19 | initial final-pass 17 horizontal replacements; extra blank paragraphs removed; repeat including picture metadata guard: 0 proposed replacements. Required full-rule line shapes retained. |
| style-check all / editable selection | 8 / 3 candidates, both exit 1. Editable three are exactly the existing permitted main 2 + setjmp 1; all candidates classified in review, not reported as raw tool PASS. |
| style-extra all / editable selection | 231 / 79 candidates, exit 0; semantic dispositions in review, not raw exit-code compliance proof. |
| Eight inventory shell runners `sh -n FILE` | all exit 0 / no guest execution |
| `git diff --check` | exit 0 |
| Existing OS boundary checker, frozen disk-image/stamp | C1–C5 and L1–L5 PASS / exit 0 |
| Host probe `cc -O2 -g -Wall -Wextra -Werror ... $(pkg-config --cflags --libs wayland-client)` | compile exit 0 / warnings 0 |
| Probe bad argument / missing display | exits 2 / 1 as expected; no compositor/guest started |
| Four host constructor/listener refusals | ENOMEM for pool/buffer/frame and EINVAL for listener; exact owned pool/buffer/callback cleanup counts, exit 0 |
| `sh plan/ws094/tests/build-probe.sh build/b2-ws094-p011-inset` | target compile exit 0 / warnings 0 |

The constructor refusal harness is a private verification helper at
`build/b2-ws094-p007/probe-failure-host.c`, included by hash in evidence. It
includes the actual probe, wraps the system Wayland proxy constructor/listener
and destroy calls using GNU `--wrap`, and uses real 64x64 shm. It verifies only
these local acquisition/cleanup boundaries; it does not substitute for actual
compositor protocol, input, rendering or restart tests. No new product test
control was introduced.

Target command, first with `-n` and then without it:

```sh
make -n -j16 \
  -o /home/awe/zedBSD-worktrees/b2/build/b2-ws094-p011/toolchain-cache/sysroot/.zedbsd-sysroot-complete \
  ZEDBSD_TARGET_SYSROOT=/home/awe/zedBSD-worktrees/b2/build/b2-ws094-p011/toolchain-cache/sysroot \
  ZEDBSD_SYSROOT_AMD64=/home/awe/zedBSD-worktrees/b2/build/b2-ws094-p011/toolchain-cache/sysroot \
  ZEDBSD_CONFIG=plan/ws102/tests/config-amd64-inset.mk BUILD=build/b2-ws094-p011-inset \
  build/b2-ws094-p011-inset/bin/files build/b2-ws094-p011-inset/bin/wayland \
  build/b2-ws094-p011-inset/bin/imageview build/b2-ws094-p011-inset/dynamic/libkeiland.so
```

Inspected final dry-run: 130 own BUILD compile/link/ELF-check lines, no shared
write, sysroot regeneration, LLVM/Noct/package build/install. Existing q582
private sysroot/current Vulkan header overlay reused unchanged. Probe's dedicated
runner separately uses the existing read-only cached target sysroot; it performs
only a compile, not cache regeneration. Target clang 23.1.0, host GCC 14.2.0,
Python 3.13.5, clang-format 19.1.7.

Boundary environment uses the same ZEDBSD_CONFIG/BUILD/private sysroot values and
`MAKEFLAGS='-o disk-image -o <private stamp>'` with
`sh plan/tools/keiland-os-boundary/check.sh`. Earlier missing-config and recursive
make dry-run failures are preserved in checkpoint 1; no package build/install
completed. Final frozen check inspects expanded membership without image recipe
execution.

## Remaining work and cessation

Read-only compositor/public-header/Files thumbnail/context/Image Viewer findings
remain in the review table and raw candidate evidence. They require an approved
ownership scope after integration of the concurrent compositor work, followed
by final source revalidation. Whole p007 also retains guide.md's full guest
desktop/probe twice, p010, Files 14 regressions, C9, physical p012 and final boot.
No QEMU, fresh image, native full package build or hardware test ran in q588.

2026-10-02 user instruction via main: B1/B2 finish the current Phase, wrap up and
exit. q588 ends at this scoped result; no q593 implementation starts. Background
asset/default decisions and investigated paths are retained in
[q593-resume.md](q593-resume.md). q593 remains reserved/unexecuted; WS099 p019
planning at B main `706d2460` is not implementation authorization.

All q588 shell/build/test processes finished. No guest was started or QEMU lease
acquired. Existing q582 owned QEMU/renderer were previously stopped and their
runtime session record removed. Private build/evidence caches remain for resume;
shared toolchain and source trees were read-only. Main owns Queue/WS projections
and final integration/ACK; B2 does not alter A-owned shared records or push.

## Main integration / 2026-10-02

Final submission `111b864a` was integrated at `9323725b`; lane ACK `04f05ff1`.
Main reviewed the source/manual dispositions and verified all nineteen original
log hashes. q588-i01 is cleared for its approved partial scope; whole p007 remains
uncleared. The sole merge conflict was the Phase status projection, reconciled
to uncleared. B1's concurrent compositor changes require the recorded inventory
and final runtime checks to be refreshed before whole-Phase acceptance.
