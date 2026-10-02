# q588-i01 / full-standard source review

2026-10-02 / B2. Full authority: `plan/coding-style.md`, Guardrail,
`plan/standards/automation.md`, and the initializer-required declaration exception
dated 2026-10-02. No concise C replacement or new exception was introduced.
The [approved partial scope](q588-approved-scope.md) determines edit ownership.

## Inventory and review boundaries

[q588-inventory.json](q588-inventory.json) contains 56 current paths, each with
SHA256, git blob, line count, original path, source origin commits, follow-up
commits, ownership and manual scope. The inventory unions non-merge commits
recorded by WS094 Phases and follows the public header to
`userland/desktop/keiland/keiland.h`. Commit `cec34d3e` is a later WS104 relocation,
not the origin of every desktop source it touched; treating it as an origin would
incorrectly import unrelated applications. Six WS094 shell test runners and eight current native Makefiles are
included as supporting source-registration context.

Review covers complete WS094-created C files and tests and the WS094 additions
and changed function paragraphs in shared files. It does not claim a full review
of all unrelated legacy code in the large compositor/Files/Image Viewer files.
Origin changes include p002 `2c82629d`, p005 `fa88c205`, p013 `6acfb616`, desktop
client/startup/timing changes `79038551`, `608987c6`, `948775b9`, and their current
follow-ups listed in the inventory.

| Current source group | Manual coverage / conclusion |
| --- | --- |
| Files desktop-layout.c, ui-desktop.c, ui-desktop-drag.c, ui-desktop-actions.c | Complete files: saved layout parsing/writes and ownership, full-list pruning, cell/label/render cache transitions, rename, context actions, drag/drop and touch. Ordering, comments, clause shapes, explicit exits and individually checked results corrected within ownership. Rendering/layout policy and normal operation order preserved. |
| Files main.c, present.c, window.c, files.h, window.h | WS094 desktop role startup/options, main_startup type and samples, instance creation, first-frame/menu ordering, selection timing, role configure/close and desktop/cache state/declarations. No API or unrelated window listener refactor. |
| picture/picture.c, picture.h | Complete shared EXIF/orientation/JPEG/GIF source. Checked bounds, allocation lifetime, orientation index transforms and error conventions. Meaningful field/colour names, declarations, public introductions and explicit exits corrected. GIF metadata status now checked immediately; missing/invalid control metadata keeps NO_TRANSPARENT_COLOR, preserving the existing fallback. |
| libkeiland/desktop.c | Complete protocol descriptions, manager search, constructor/listener ownership and dispatch. Callback declarations before tables are initializer-required. Newly checked registry, listener and roundtrip failure paths release registry/wrapper/private queue and return NULL; the successful protocol request order is preserved. |
| tests/desktop-probe.c | Complete probe: protocol tables, registry/seat/data-device/surface/frame constructors and listener results, roundtrip/dispatch/flush/poll/time results, shm FD/map/pool/buffer/callback cleanup, argument/timeout handling and normal observation messages. Added safe setup failure exits; no new protocol request or test control. Final disconnect releases connection-owned proxies/queued events. |
| tests/host-desktop.c, host-thumb.c | Complete existing tests. Allocations and layout operations checked individually; fixture comparisons/expectations and successful operation order retained. Thumbnail output checks each fwrite and final fclose before releasing pixels; no weakened expectation. |
| Files menu.c, ui-context.c, thumb.c | Read-only p005 menu/model integration and p013 JPEG/GIF wrappers. Findings below remain for a later owned attempt. |
| Image Viewer image.c | Read-only p013 decoder extraction/integration, orientation wrapper, GIF first-frame/animation ownership and current integration paragraphs. Findings below remain. |
| wayland/desktop.c, desktop.h | Complete new desktop role module/header: token/role validation, placement/configure/frame/render handling, focus/input, spawning/restart and environment switch. Read-only; genuine conformance findings retained below. |
| wayland compose.c/h, data.c, display.c, main.c, menu.c, menu-shell.c, objects.c, protocol.c, shell.c, touch.c, zwl.h | Current p002 desktop layer/focus/input/role/object integration and p005 desktop context-menu routing hunks; inspect current declarations/call sites with the origin diffs. No compositor edit or runtime validation. |
| tests/*.sh | Six read-only runner setup/build/install source membership, bounded assertions, screenshots/log capture and guest cleanup contracts inspected; host runners executed, all eight inventory shell parses pass. No guest runner executed in q588. |
| current keiland/keiland.h; libkeiland/exports.map | Desktop API signatures/listener and exported symbols correspond to the implementation; no public ABI edit. Non-desktop header candidates are outside WS094 review. |
| Files, wayland, libkeiland, imageview Makefile and each Makefile.linux/.freebsd | Desktop/picture sources registered once in each applicable source list; matching decoder link dependencies and desktop exports retained. Native files are review context, not native compile results. |
| platform/amd64/vmunix.mk; plan/tools/files/host-build.sh; plan/tools/imageview/run-host.sh | p013 link/ELF-needed registrations and host decoder source lists inspected. Target/host builds validate the actual memberships; shell changes were read-only. |

## Full-rule semantic checks

Review considered file ordering, ANSI declaration placement and syntax, public
introductions/signatures and single-line prototypes, descriptive names, variable
and field purpose, semantic paragraphs and loops, unused callback parameters,
short-circuit order, clause/argument layout, immediate individual fallible-result
checks, allocation/resource lifetime, debugger-visible decisions and separate
failure/success exits. Existing licenses and API/module/OS boundaries were
preserved. Formatter/mechanical checks supplement this review.

JPEG status discards were inspected against the actual target decoder contract:
`userland/base/libjpeg-compat/decompress.c` returns JPEG_HEADER_OK with
require_image=TRUE, TRUE for start/finish, and one scanline while output_scanline
is below output_height; errors call error_exit and reach the existing setjmp
cleanup. No input suspension occurs for these complete file/memory sources.
Those status discards do not hide an independently reported target failure.
Host tests also exercised the corresponding complete input fixtures; this is not
a claim about arbitrary suspended upstream-libjpeg sources.

Cleanup operations and diagnostic writes do not supply a new success result on
an existing failure path. The probe retains EAGAIN/EWOULDBLOCK backpressure and
EPIPE until readable server protocol error dispatch, matching the current
libwayland flush contract. The target and host constructor wrappers differ by
libwayland ABI version; host failure injection is explicitly bounded below.

## Mechanical findings and disposition

Durable raw results are in [q588-evidence](q588-evidence/). `style-check.py` reports
8 candidates, exit 1; the editable selection reports exactly the existing 3
permitted findings. `style-extra.py` reports 231 candidates, exit 0; its return
code alone is not a pass. The editable selection has 79 candidates, all two
protocol table false positives or unrelated legacy portions of shared files.

| Finding / current location | Classification / ownership / next action |
| --- | --- |
| main.c:229/642 blank-after-brace; picture.c:294 setjmp | Existing p007 exceptions; matched by content to p008/p009 and p013 evidence. Scope not expanded to newly generated code. |
| compose.c:358/373/403 blank-after-brace | Read-only existing frame-log paragraphs, retained as broader compositor conformance candidates; not an exception for B2 changes. |
| display.c:243 zwl_desktop_is in condition; menu.c:843 zwl_desktop_surface in condition | Genuine WS094 integration violations of separate call/check rule. Read-only B1/compositor ownership; later approved conformance attempt needed. |
| desktop-probe.c:122/139; present.c:959–963; window.c:131/132/138; menu-shell.c keyboard arrays | False split-argument candidates: interface/vertex/listener/key table initializers, not function definition arguments. |
| main.c purpose candidates and argv[0] condition; window.c legacy unused casts; public keiland.h non-desktop signatures | Unrelated pre-WS094 legacy ranges. main argv condition traced to 00f8a8d2 before WS094. Do not imply these files are globally compliant. |
| thumb.c:97/550/555 clause shapes | Existing three p013/p007 style-extra exceptions. Unchanged content; no generalized waiver. |
| thumb.c:645 compound Boolean return; thumb_gif_source type at685; thumb_gif immediate result check after cleanup at722; thumb_adopt at759 paragraph/explicit exit | Genuine p013 conformance residuals, but thumb.c explicitly read-only in q588. Later Files/picture owner scope required. |
| image.c:223 iv_image_orientation success return paragraph; image_jpeg at433 result checked after fclose | Genuine p013 return/immediate check residuals, read-only Image Viewer ownership. |
| ui-context.c:314 context_desktop final explicit success exit; desktop.c void public/static exits and desktop_word:827 return comment | Genuine WS094 full-standard residuals in read-only scopes; later owned conformance work. |
| desktop.c:797 three clauses on one line; desktop_get:539 significant nested word/find calls; desktop_start spawn result checked after starts increment; desktop_switched_off read result checked after close | Genuine read-only control flow/result layout findings; B1/compositor conformance scope required. |
| Remaining all-source style-extra candidates | Read-only shared legacy/p002/p005 paragraphs with condition shapes, log argument continuations, direct meaningful-call returns, unused casts and purpose/declaration candidates. Raw path/line candidates retained; no ownership waiver and no whole-Phase clearance. Compositor/native/public header owners must resolve applicable WS094 candidates against their then-current source. |

No unresolved genuine violation was found in the reviewed editable WS094 scope
after these corrections. This supports only q588's partial item. The read-only
residuals and the remaining runtime/hardware gates prevent whole p007 clearance.
