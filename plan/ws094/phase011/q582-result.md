# q582-i01 / B2 checkpoint

Status: in-progress / Phase clearance pending
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
- Three pre-existing blank-after-brace findings in the touched desktop C files are repaired by whitespace only.

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

Complete private-sysroot target build/image build, wait for B main guest-resource grant, execute prune/L1/drag (+menu) focused regressions, run boot-test and show its PNG. Record final command versions, results and commit. QEMU evidence is distinct from real hardware; no real hardware work is in q582. No push/GitHub publication.

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

Final `host-desktop-final.log` PASS includes failed-listing→successful-listing prune without forcing cache invalidation. Style/diff-check remain PASS. Guest startup waits for B3 to release Venus resources; boot-test and functional regressions remain unverified.
