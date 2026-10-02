# q588-i01 / source conformance checkpoint 1

2026-10-02 08:15 UTC / B2. Base `60201ab88cbf482024a70db9bcae4f2e2d771a45`.
Exact scope: [approved partial scope](q588-approved-scope.md), SHA256
`a1faf4833c5289290f6ae2c42170d8be80ff0ab39b3520a04f033ec8f157a6f5`.
This is an intermediate source checkpoint; inventory/manual review and the
partial attempt outcome are still pending. Whole p007 remains uncleared and
WS094 incomplete; physical p012, the complete guest suites and final boot remain.

## Changes and behavior review

- Files desktop layout/UI: put public definitions before static helpers and
  LABEL_ELLIPSIS with constants; expose three-or-more-clause conditions on separate
  lines, preserving short circuit order. Document explicit void success exits and
  nested traversals. A band-end log stores its tab/selected index before logging.
- WS094 Files startup/selection timing logs: one argument per continuation line;
  purpose comments for persistent desktop folder/startup state. All format strings,
  arguments, timing samples and startup operations remain in their original order.
- `fm_present_instance`: keep `vkCreateInstance`'s VkResult convention, expose its
  failure check and successful return. Separate application and instance setup.
- libkeiland desktop: retain only initializer-required callback prototypes ahead
  of constant listener tables; ordinary desktop_bind declaration follows tables.
  Use UNUSED_PARAMETER before semantic paragraphs. Immediately inspect constructor
  output, keeping manager destruction on both paths before listener setup/free.
- Shared picture decoder: explicit callback argument markers, field_value /
  colour_index role names, multiline public introductions, one-line declarations,
  purpose comments and explicit void exits. EXIF/GIF bytes and decoder flow unchanged.
- Existing desktop host test: check allocations separately; replace Boolean
  assignments/return with explicit decisions, preserving comparison results and
  first-line-before-second-line text-width short circuit.

No public API, desktop drawing policy, file formats, OS layer, compositor source,
shared tool or build rule was changed.

## Verification

Logs are under `/home/awe/zedBSD-worktrees/b2/build/b2-ws094-p007/`.

| Check | Command / result |
| --- | --- |
| Desktop host | `sh plan/ws094/tests/host-desktop.sh` / exit 0, host-desktop PASS |
| Thumbnail host | `sh plan/ws094/tests/host-thumb.sh build/b2-ws094-p007/host-thumb` / exit 0, all fixtures PASS |
| Model host | `sh plan/tools/files/host-model.sh` / exit 0 |
| Image Viewer host | `sh plan/tools/imageview/run-host.sh build/b2-ws094-p007/imageview-host` / exit 0 |
| Target build | q582 private sysroot, frozen .zedbsd-sysroot-complete, `make -j16`, BUILD=build/b2-ws094-p011-inset; bin/files, bin/wayland, bin/imageview, dynamic/libkeiland.so / exit 0, warnings 0 |
| Safe dry-run | Same target set with `make -n` / own BUILD compile/link only; no sysroot replacement, LLVM/Noct/package build or install |
| Owned style | style-check on 12 owned/related C/H plus host-desktop.c / exactly existing main.c blank-after-brace 2 and picture.c setjmp 1; no new finding |
| Formatter | clang-format-19 19.1.7, InheritParentConfig + ColumnLimit 0, git-diff edited ranges / 12 horizontal replacements, no structural replacement proposed; full-rule line shapes retained |
| Diff | `git diff --check` / exit 0 |
| OS boundary | existing check.sh with ZEDBSD_CONFIG and MAKEFLAGS freezing disk-image and private stamp / C1–C5, L1–L5 PASS, exit 0 |

Boundary invocation history is preserved: plain check.sh failed before inspection
with missing config.mk; configured retry failed at recursive make's missing private
OpenSSL build directory. GNU make executes recursive make recipe lines even with
`-n`; no package build/install completed. Final invocation freezes disk-image
itself with `MAKEFLAGS='-o disk-image -o <private stamp>'`, so the existing checker
reads expanded membership without visiting image prerequisites. This does not
claim image construction or runtime validation.

Compiler/host versions are the q582 toolchain: clang 23.1.0, host GCC 14.2.0,
Python 3.13.5. Formatter 19.1.7. Shared cached sysroot is unchanged; the existing
private copy carries only the documented current Vulkan header overlay from q582.

## Next work

Complete commit-derived inventory with current hashes and review boundaries,
read-only compositor/header/imageview/build ownership findings, complete owned
host/probe source review and final revalidation. Background image discovery from
new user direction is read-only planning for a separate subsequent Queue.
