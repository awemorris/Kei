# q582-i01 / B2 checkpoint

Status: cleared / q582-i01 cleared (2026-10-02 07:43 UTC)
Start: 2026-10-02 07:13 UTC / maximum 3 hours
Base: ea55c9739d7c5e179297271c11add7ca4b1252b8
Worktree: /home/awe/zedBSD-worktrees/b2 / codex/b2-ws094
Approval: current user 2026-10-02「では、N=3で作業を開始してください。」; B main selected WS094 p011, canonical lane `plan/agents/B2/queue.md`, approved snapshot SHA256 e3bca2fe809cf8af60f705454a43d43b677153221f8486922d8bd304686a0f77.

## Changes

- `desktop-layout.c`: prune saved places using the complete successful listing; retain names outside the visible grid; compact memory without writing the layout file. The next placement/rename persists it.
- `ui-desktop.c`: guard pruning with `listing.error == 0`; append `hidden=` to the existing ready line. Rendering policy is preserved.
- `files.h`: internal prune declaration and cached listing error. The failed→successful listing transition refreshes pruning even when count/names are unchanged; the host test does not invalidate the cache manually.
- `host-desktop.c`: persisted stale-row removal, retained off-grid name, repeated/empty listing, deferred write, failed-listing guard.
- `files-desktop-guest.sh`: new prune step checks a stale row before/after dragging, hidden=0 and 100 items/91 cells/hidden=9. Guest execution pending.
- Three pre-existing blank-after-brace findings in the touched desktop C files are repaired with purpose comments and spacing; behavior is unchanged.

## Validation

Commands run from the B2 worktree. Evidence is under `build/b2-ws094-p011/`.

- Baseline host-desktop initially failed two font checks because worktree assets were absent. Copying existing Inter/JetBrains fonts and wallpaper from claude1, plus host Droid fallback/license into B2 build, recovered baseline PASS. No product source fix was needed.
- `sh plan/ws094/tests/host-desktop.sh`: PASS, including added prune checks (`host-desktop.log`).
- `sh plan/tools/files/host-model.sh`: PASS (`host-model.log`).
- `sh plan/ws094/tests/host-thumb.sh build/b2-ws094-p011/host-thumb`: PASS (`host-thumb.log`).
- `python3 plan/tools/style-check.py` for the three production files and host test: baseline 3 pre-existing findings; final 0.
- `git diff --check` and `sh -n plan/ws094/tests/files-desktop-guest.sh`: PASS.
- Initial target build using frozen shared sysroot failed in existing libvulkan because cached public Vulkan headers omit current KHR memory requirements definitions (`bin-build.log`). Files changed source compiled. No shared tree was modified.
- B main authorized copying sysroot to B2 private build and reflecting the current public Vulkan headers. Overlay file/hash evidence: `sysroot-vulkan-overlay.json`; private path `build/b2-ws094-p011/toolchain-cache/sysroot`. Dry runs prevent sysroot regeneration and LLVM/Noct rebuild.

## Remaining

q582 implementation and required verification are finished. WS094 remains incomplete; p007 full-WS conformance, p012 hardware acceptance and p009 follow-up are outside this Queue. Fresh full-image generation was skipped using the B-main-approved existing fixture because it would rebuild forbidden Noct/toolchain/package dependencies. QEMU evidence is distinct from real hardware; hardware checks are unperformed. No push/GitHub publication.

## Build / fixture checkpoint

Private-sysroot target build and final rebuild after the cached-error correction: exit 0, warnings 0 (`bin-build-private.log`, `bin-build-final.log`). Exact arguments:

```sh
make -j16 \
  -o /home/awe/zedBSD-worktrees/b2/build/b2-ws094-p011/toolchain-cache/sysroot/.zedbsd-sysroot-complete \
  ZEDBSD_TARGET_SYSROOT=/home/awe/zedBSD-worktrees/b2/build/b2-ws094-p011/toolchain-cache/sysroot \
  ZEDBSD_SYSROOT_AMD64=/home/awe/zedBSD-worktrees/b2/build/b2-ws094-p011/toolchain-cache/sysroot \
  ZEDBSD_CONFIG=plan/ws102/tests/config-amd64-inset.mk BUILD=build/b2-ws094-p011-inset \
  build/b2-ws094-p011-inset/bin/files build/b2-ws094-p011-inset/bin/wayland \
  build/b2-ws094-p011-inset/dynamic/libkeiland.so
```

Compiler: project clang 23.1.0 (d7f1bbaca898fb5f4cc373b082e915ec1a07310f); host GCC 14.2.0 (Debian 14.2.0-19). Private sysroot is a real copied directory, not a symlink. Only `vulkan_external.h` differs from the existing cache: cached SHA256 `241ddecf632ebf9cfdb153c892eb015b21e007996bed2f5409189014f4f9390f`, current SHA256 `2a708c008fc4eebade71d9df4d16d202b9c7f1df5c1d504c2b40ffdb8ca4c3e2`. B main authorized the private overlay. Shared LLVM/Noct/sysroot files were read only. Dry-run with private stamp had no sysroot regeneration or LLVM/Noct build.

Image generation was not run: inset disk-image dry-run planned forbidden Noct source/build operations and lacked the OpenSSL stage. B main authorized reusing the existing q577 fixture `/home/awe/zedBSD-worktrees/p8/build/p8-q577/criteria.img`, SHA256 verified `992e83f498cda2a1d506bb6ad7975b3c4592069fcb6493eca707e0d84403ac37`. Independent non-symlink copy is `build/b2-ws094-p011-inset/hdd-image.img`. The fixture kernel/base differs from a fresh current-HEAD image and will remain reused; guest install will supply current Files/wayland/desktop libraries. Fresh full-image build is unverified.

Final `host-desktop-final.log` PASS includes failed-listing→successful-listing prune without forcing cache invalidation. Style/diff-check remain PASS. At the second checkpoint, guest startup waited for B3 to release Venus resources. Final guest and boot results follow below.

## Final verification / clearance

Finished: 2026-10-02 07:43 UTC, within the 3-hour bound (approximately 30 minutes including resource wait). Implementation tip: `45f8d2da21093886e3ecd0114c3fa9059d80482b`; B-main integration ACKs: MR01 `c84646b0`, MR02 `99e41f12`.

All guest commands used `GUEST_RUNTIME=/home/awe/zedBSD-worktrees/b2/build/b2-ws094-p011-run`, `BIN=build/b2-ws094-p011-inset`, and the existing strict Venus renderer `/home/awe/zedBSD-claude1/build/ws035-sq-venus/install`. Dedicated QEMU started only after B3 released the shared resource. SSH was through host loopback; no QEMU console/serial logs were read for assessment.

| Check | Command / evidence under build/b2-ws094-p011 | Outcome |
| --- | --- | --- |
| Current host layout | `sh plan/ws094/tests/host-desktop.sh` / host-desktop-final.log | PASS, including failed→successful listing recovery |
| Current host model | `build/ws071-host/files-model` against a disposable test directory / host-model-final.log | PASS (script also passed before final cache-field adjustment) |
| Thumbnail decoder | `sh plan/ws094/tests/host-thumb.sh build/b2-ws094-p011/host-thumb` / host-thumb.log | PASS |
| Current target build | Exact private-sysroot command above / bin-build-final.log | exit 0, warnings 0 |
| Full C rules on changed code | Manual review against coding-style.md; style-check.py on desktop-layout.c/ui-desktop.c/files.h/host-desktop.c | PASS, tool total 0 |
| Whitespace / shell | `git diff --check`; `sh -n plan/ws094/tests/files-desktop-guest.sh` | PASS |
| New pruning / overflow | `sh plan/ws094/tests/files-desktop-guest.sh build/b2-ws094-p011/prune install show saved prune` / prune-result.log | PASS, ERROR/FAILED absent |
| Existing L1 | same script `build/b2-ws094-p011/l1 install show watch input saved` / l1-result.log | PASS |
| Existing drag / DnD | same script `build/b2-ws094-p011/drag install show drag` / drag-result.log | PASS |
| Existing menus | same script `build/b2-ws094-p011/menu install show menu` / menu-result.log | PASS, including rename/Trash/copy/paste/New Folder/Clean Up |
| Fixture baseline boot | `OUTPUT=build/b2-ws094-p011/boot plan/tools/boot-test.sh build/b2-ws094-p011-inset/hdd-image.img` / boot-result.log | PASS; login PNG viewed and shown |
| Final staged boot | sync guest, stop guest, copy its updated disk to staged-image.img; `OUTPUT=build/b2-ws094-p011/boot-final plan/tools/boot-test.sh build/b2-ws094-p011-inset/staged-image.img` / boot-final-result.log | PASS; login PNG viewed and shown |

`prune/pruned-layout.txt` contains only `notes.txt<TAB>3<TAB>3`: `ghost.txt` existed on disk before the ordinary drag/save, then disappeared. `prune/prune-log.txt` records `prune removed=1 kept=1`, `items=5 cells=5 ... hidden=0`, and `items=100 cells=91 ... hidden=9`. `prune/prune.png` and `prune/overflow.png` were visually inspected; the overflow screen preserves the existing display policy.

Artifact SHA256:

- `prune/prune.png`: `6a86a710ba726feff41d379244346e2c6da51d3ec284a949864b4bd91c79eebb`
- `prune/overflow.png`: `76db16d896e34e7fef58434b208eb84cf64621a58dd92512663dfd8cec73bde8`
- `boot-final/login.png`: `b82368dc81cc7b9105d8fa146b95380d9d316ba1045da13a12394dceccbee00d`
- `build/b2-ws094-p011-inset/staged-image.img`: `83eb025d9b78e665caff6009450944abf4462dfb52d48f832b50eaaa7227c56d`

Environment: QEMU 10.0.11 (Debian 1:10.0.11+ds-0+deb13u1), Python 3.13.5, KVM, Lavapipe/strict-queue Venus renderer. The fixture kernel/base was reused rather than rebuilt; final staged image contains current target Files/wayland and desktop libraries. No compositor/Settings/libkeiland source, other WS source/tests, HAL API, shared build or toolchain source was edited. Existing libkeiland/libvulkan were built into B2 private BUILD as dependencies, not changed.

The owned Venus guest was stopped after sync. Final framebuffer boot cleaned itself up. Read-back found no qemu-system or virgl_render_server processes, and the B2 runtime session.json is absent. No remaining q582 implementation work or newly deferred bug. Phase and attempt criteria are satisfied; WS acceptance is still incomplete. Shared Queue/Master/history projections and GitHub publication belong to Agent A/B main.
