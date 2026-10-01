# WS105 final source conformance — q538 / Q1 / 2026-10-01

Full standard: [coding-style.md](../../../coding-style.md), SHA256 `2244803c00d4a3346ab4f74c227f16a5b65c439759dfc1184697ab9f63df3930`.
Guardrail and AGENTS.md are authoritative. No concise C standard exists: the full document was loaded. [Coverage](../../../standards/automation.md) records automation limits.

Final source: `c7e8a35a30f138b766385f2b48183540821c325b` (WIP), base `39a0941c^` / WS104 completion `d67037af`. [Exact source manifest](source-SHA256SUMS) covers 96 changed production/build/tool/doc paths. Existing common files were reviewed over their WS105 diff plus the surrounding contract; full new Linux, WPA, libvulkan-compat and reusable fixtures were reviewed. Human-owned IME source was not edited (the existing 13 sources are compiled by Makefile.linux). Locked target toolchain / sysroot policy / Noct were not changed.

## Review of all applicable full-standard rules

| Full rules | Final review / verification |
| --- | --- |
| §1–§4 organization, declarations, language | C file sections, type/global lifetime comments, public definitions before static functions, one-line static declarations, multiline verb/object public comments. Function-leading declarations; no declaration calls, nested declaration groups, for-init declarations or scope-only blocks. Integer constants and format strings use uint64_t / UINT64_C / PRIu64 / PRIx64 / SCNu64 instead of ordinary long-long syntax. Reserved compiler attributes are explicit |
| §5, §7–§8, §10 paragraphs / loops / braces / calls | Scoped clang-format19 followed by required per-argument definition / compound-decision restoration. Immediate fallible checks, purpose paragraphs and lock-body spacing; multiline controlled bodies and terminal else symmetry; comments describe domain actions and observed counter/lifetime meaning |
| §6, §11 decisions / returns | Actual significant calls are separate from conditions and initializers. Boolean producers use decisions; short-circuit order preserved. Meaningful call results have refusal/success branches with the actual convention (errno, count, VkResult). Void success returns and public comments supplemented during p011. Generated forward.inc preserves original handle/argument/result and VkResult non-success; its generator was reviewed |
| §9 ownership / initialization | One fallible allocation and check at a time; private Wayland queue and callback lifetime; oldSwapchain buffers / implicit reservation fallback; KMS outstanding flip and caller/master descriptor ownership; compositor GPU params and buffer fd retirement after Vulkan use; D-Bus SCM_RIGHTS framing and rejected-frame fd recovery; logind revoked-input lease until ResumeDevice; WPA watch registry and normal process destruction; ALSA watch/ctl closure |
| §12 diagnostics / production selection | No test-only environment controls in production. NO_IMPLICIT_SYNC absent. CPU fallback follows actual ioctl capability/failure. Test-only preload/import observers stay in fixtures; NO_DEEPBIND/DRM_DEVICE/SEAT are documented configuration/diagnostic interfaces. Host DRM_DEVICE=none and no host display/input used |
| §13 license | New implementations have zedBSD 2026 Zlib source headers. System Vulkan/kernel headers are used through the prescribed build boundary. No external implementation copied; existing OpenBSD digest sources retain their own license |
| Guardrail / WS policy | C1–C5 + L1–L5 OS/module/build checker; Makefile.linux isolation and source-sync; no uapi/networkd/audiod includes in Linux/WPA, no Linux DRM/ALSA includes in zedBSD; libvulkan-compat never compiles target Vulkan source. Public desktop API and user decisions D1–D25 retained. No host package added or host /opt install |

## Machine checks and their limits

- `clang-format-19 '-style={BasedOnStyle: InheritParentConfig, ColumnLimit: 0}'` over edited/new scope, then manual restoration. Version 19.1.7; no unrelated mass reformat.
- `python3 plan/tools/style-check.py <all new C/H + dbus-wire.c + seat-fd.c> --summary`: **total 0**. This tool misses some public-comment/declaration cases; formatter/tool success is not the full review.
- Compiler AST review of **32 C translation units**: actual call initializers / conditions / direct returns and late declarations: **0 findings**. libc `errno`'s `__errno_location()` macro expansion is classified as reading the value, not a significant call.
- Structural AST review: no standalone scope block / ordinary void missing success return / Boolean producer findings. One `compat_missing` end is `abort()` and is deliberately non-returning: it cannot have a reachable success result. Not a waived runtime failure.
- Shell syntax uses the declared interpreter (wsi-check.sh is bash); Python AST syntax PASS. The first blanket `sh -n` attempt rejected bash syntax, an invalid harness invocation; no production edit was needed.
- `git diff --check`: PASS. No new authoritative formatting config or policy exception was introduced.

## Build and host behavior on the final source

Clean gcc 14.2.0 and clang 19.1.7 builds with GNU Make4.4.1 / glibc Debian13: warning0; ELF24 production each with RUNPATH /opt/keiland/lib; source-sync and 331-source header checks PASS. The initial clean build exposed colliding object variable names for library/program wayland; p011 fixes the internal variable namespaces. The original failure observation excerpt is kept (the first full build log had been overwritten; no full-original-log claim). Each subsequent clean build passed.

Headless host Vulkan chain 1MiB /262144words, interpose bindings0 and NO_DEEPBIND opt-out, vulkaninfo: PASS. Wayland WSI90frames × FIFO/fallback/resize/MAILBOX: every extent/pixel, actual kernel import observer and CPU wait PASS. D-Bus independent5cases ordinary + ASan/UBSan: PASS. Mesa25.0.7 / Vulkan loader1.4.309; QEMU10.0.11. No hardware GPU or Vulkan CTS claim.

## Guest/source revalidation and residual limits

Own fresh Debian13 gdm image / kernel6.12.107+deb13-amd64, independent base and disposable overlay. Initial complete p005–p010 run at 8fb18105: KMS/seat-fd, root desktop, wl_shm, pointer/key, wltest600, malformed buffer refusal, Home9apps, Terminal echo, actual Textedit Japanese conversion/commit, Image Viewer picture/PDF2pages, real hwsim/wpa and HDA/ALSA Settings/bar, root Home Log Out and default-socket SIGTERM PASS.

After final integer-style changes, clean both builds/host tests and affected guest paths repeated: six KMS color screens /24 exact sampled pixels /oldSwapchain /caller fd ownership, root drawing/wltest600/malformed buffer, network scan-save-join-details-disconnect PASS. First repeated network probe started from an already connected radio and correctly timed out on its required disconnected precondition; reset profiles/disconnect and repeated successfully. Original observation retained. No network repair claim.

Final gdm chooser explicitly shows/selects Keiland; manual login starts userkei PID5733, Wayland /run/user/1000. SwitchTo and chvt each keep same compositor and all5 DRM/evdev leases, 10 pause +10 resume total, restored Terminal echo PNG. Actual type **force**, cooperative ACK only source-reviewed. Home Log Out `error=0 cleanup_failed=0`, private paths0, gdm greeter restored. Own guest stopped, overlay removed. Hardware hotplug, systemd restart, cooperative pause, other GPU/vendors/musl/ARM/FreeBSD are untested or design §8 out of scope.

Initial helper expectations for bar percent23 (actual ALSA rounding20), VT message spelling, dirty Textedit confirmation and uppercase IME conversion were corrected using actual UI/contracts; original logs retained. Both dirty Textedit windows were closed via Don't Save, status0 /no Textedit process. No hidden app/source changes were made for those harness corrections.

zedBSD mandatory final regression finished: disk-image warning0, login PNG, C1/C2/C9 original11 PASS /2 resize FAIL, V1 /dedicated18 /decoder17 ordinary+sanitize, forge refusal+120frame, fence600/generation1, p059 glass, Settings8 subtests, host audio14/14, volume-p004/p005 PASS. [Phase](phase.md) and final Queue outcome retain exact evidence and limitations. Known C2/p076 resize FAIL is preserved in [BUG-125](../../../bugs/BUG-125.md) with the user's explicit non-blocking tracking/clear decision; [BUG-127](../../../bugs/BUG-127.md) remains unrepaired tracking though p072 passed. These are not general standards or test exemptions.

GitHub publication/Issue closure/Project projection remain pending in local outbox, as directed. No push. Source conformance revalidation is required if later code changes affect these results.

## Final output and cleanup reconciliation

Explicit Vulkan window (640×420) at the final source: client3 600 commits and each of clients4–8 20 commits, 3 imported buffers/client (18 total), acquire-fences700; all5 window closures fd29→29; red/green/blue actual interior pixels match, malformed-buffer out_of_bounds refused and compositor remains alive. SIGTERM `frames=716 error=0 cleanup_failed=0`; disposable overlay removed. Initial whole-compositor count701/21 also included desktop Files (1 fence /3 imports), so its aggregate-only helper assertion was invalid. Original log/script retained; later assertion counts the actual test clients. No production repair for this harness correction.

The numerical KMS timeout conversion preserves the exact5s /100ms values and algorithm; actual final KMS frames and ownership were revalidated. q530's delayed-poll regression and original failure evidence remain reachable; its injection was not repeated during q538. A base-before hash was not captured for this own fresh guest; guest controller's backing-image/overlay isolation and stopped/no-overlay condition were checked, not a fabricated base hash comparison.

Final results and selected application logs/PNGs are in [evidence manifest](evidence/SHA256SUMS). Login/logout polling retains its first/last PNG; other intermediate poll images stay in ignored build output. QEMU console/serial/kernel logs, SSH credentials and guest image binaries are excluded. [Source manifest](source-SHA256SUMS) identifies the validated final source. No remaining in-scope standards violation; declared force-VT/hardware/foreign-GPU limitations and unrepaired BUG-125/127 retained.
