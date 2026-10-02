# Standards and automation coverage

Full standard: [C coding style](../coding-style.md). Concise version not created;
load the full applicable sections before editing. Non-C areas use their established
local conventions and Phase requirements.

| Rules / check | Command / observed version | Coverage and limit |
| --- | --- | --- |
| Formatting (§3, §5, §8) | `clang-format-19 '-style={BasedOnStyle: InheritParentConfig, ColumnLimit: 0}' FILE`; 19.1.7, root `.clang-format` | Tabs, spacing and braces. Scope only new / moved implementations or edited ranges. Restore definition arguments and compound-condition clause lines required by the full standard; keep static prototypes on one physical line. Formatter output alone does not establish conformance |
| Mechanical C rules (§3–§8, §10, §14) | `python3 plan/tools/style-check.py FILE... --summary`; Python 3 | Calls in conditions, some declaration placement, missing paragraph / forward-declaration structure, multiline bodies, comment form, names, goto. Read the tool's scope and false positives; it cannot judge comment purpose, object lifetime, ownership, or every return shape. WS105 p011 found function-leading declaration and public-comment gaps; supplement with compiler AST and full/manual review |
| Whitespace | `git diff --check -- <changed-paths>` | Diff whitespace only |
| Build | `make -j64 disk-image` for WS104 amd64; GNU Make 4.4.1 | Compile/link; record configuration and classify project vs external-package warnings. Other Phases use their approved command |
| Boot | `OUTPUT=build/<W>/boot plan/tools/boot-test.sh IMAGE` | Login prompt in PNG, inspect and present it. Console / serial logs are not boot evidence |
| Runtime | Named host / guest suites in the approved Phase | Actual bounded behavior only; QEMU / Venus is separate from physical acceptance. Serialize image builds |
| Full standard (§1–§14) | Agent / human review of all WS changes, including complete new / moved implementations | File organization, lifetime / ownership comments, meaningful names and paragraphs, evaluation order, allocation / initialization, call and branch shape, success / refusal paths, licenses and test controls. Record exact source scope, exceptions and limits |
| Plan sync | `python3 plan/tools/sync.py status` | State / outbox visibility only. This checkout's GitHub publication is deferred; local records are not remotely synchronized |

No aggregate `make check`. No unrelated mass formatting. clang-format 19.1.7 was
installed on the host for WS104 (2026-10-01), without changing the locked target
toolchain or the checked-in formatter configuration. Versions here are observations,
not a new project-wide pin. The full document prevails over formatter defaults;
scoped formatting and manual restoration were recorded in WS104's final review.

HAL authority: the 2026-09-25 user decision permits implementation changes to
existing HAL declarations. Changes to `include/hal/hal.h`, its API / contract or HAL
responsibility require approval of the exact diff. This narrows the earlier
2026-09-12 instruction; [Guardrail](../guardrail.md) records the current rule.
A formatter or build never supplies that approval.


## WS105 Linux verification coverage（2026-10-01）

| Existing rule / contract | Check | Limit |
| --- | --- | --- |
| OS and build boundaries | `sh plan/tools/keiland-os-boundary/check.sh` C1–C5 + L1–L5 | Actual source includes, platform conditionals, Linux source inventory and expanded target Makefile membership; ownership still needs source review |
| Independent Linux build / installed ABI | clean gcc14.2 / clang19.1.7; `elf-check.sh`, `header-check.sh`, `makefile-sync.sh` | ELF24 production each, source331; excludes Qt/GTK/browser and other design §8 scope |
| Vulkan / Wayland synchronization | `interpose-check.sh`, `wsi-check.sh` | Headless host only; 1MiB full-word check, bindings0, 90frames×4 with actual pixels / kernel-import observer / CPU wait; no hardware GPU assertion |
| Descriptor lifetime and framing | `dbus-wire.c` / `.py` ordinary + ASan/UBSan; guest `seat-fd.c` | Independent input serialization / SCM_RIGHTS ownership; actual guest master validation. Real cooperative pause notification remains unobserved |
| Network / audio public contracts | guest `network-probe.c`, `audio-probe.c`, Settings / bar UI | Simulated radios and HDA, real services; IP/DHCP outside Keiland, PCM feedback silent |

These implement existing WS105 decisions, not additional project-wide style policy. The full C standard and formatter configuration remain unchanged.


## Browser / libbrowser 境界の追加（2026-10-01）

Authoritative full rule: [browser-component.md](browser-component.md)。ユーザー指示により WS074/WS107 に適用。
簡約版無し、既存 C 全文/formatter の規則は変わらない。

| 規則 | 検証の計画 | 現在の coverage / 限界 |
| --- | --- | --- |
| engine/source 所有と shell のみの app | tracked inventory、expanded Makefile source、private include の dependency | WS107: 全179台帳/165move、engine133/app7、全hash/mode/Makefile/includeを照合。新checkerは未実装 |
| libbrowser の Wayland 使用禁止 | public header standalone compile、include/link closure、ELF NEEDED/undefined/export | WS107: C89/C++11 header、全133include closure340、ELF39exports/NEEDED/undefined、実public client verified |
| 標準 Vulkan と抽象入力 | Wayland 無しの第2 client の Vulkan 描画、shell adapter/複数 view の契約検証 | WS107: full/manual reviewとpublic client plain/ASan各83、actual lavapipe/caller record-fence。same-view callback mutationは外側call後の契約 |

WS106 では source 移動後の既存 Makefile/source checker の locator を更新する。WS108 の distro checks はnative/QEMU/runtime/CI gateを検証済み、WS109 の FreeBSD checks は設計中、
WS105 の Linux checks の合格を新 target の合格に転用しない。各 WS の near-final Phase で全文規約/実環境/tool version と限界を記録する。

## WS106 relocation coverage (2026-10-01)

[User-authorized scoped exception](ws106-relocation.md): preserve existing implementation style for path-only relocation. Validate all move hashes/modes and allowed include/path diffs; compare normalized style-check baseline (59 C,1300 candidates). New C/semantic changes remain subject to the full standard. Review all WS source/config/runner diffs, registry/default/dependency/install contracts and final build/boot. No mass formatting.

WS107: [限定例外](ws107-relocation.md)の不変moveをhash/mode/path-only diffでreview。変更componentと新試験はC全文/edited-range clang-format/manual review、既知14候補を修正し全133C style-checkを比較。

## WS108 packaging coverage (2026-10-02)

| Existing rule/contract | Actual check | Limit |
| --- | --- | --- |
| Native distribution/isolated roots | pinned inputs hashes + OS/arch, QEMU separate8GiB overlays, source allowlist, both requested make | actual KVM full targets; TCG native build/runtime separately verified; no host install |
| Runtime/private ABI/licenses/user data | elf-check19, independent40 payload/control/md5 audit, fresh install/upgrade/remove, public Vulkan/direct GUI/real input | physical/display-manager startup not re-certified |
| CI/release fail gates | verify-artifacts.py, YAML/bash/deep-equal original build, actual merged artifacts and rejection of corrupt/missing input | remote Actions/release unexecuted |
| Final full standards | all8 source files manual full review + Python byte compile/git diff-check; no new C | [full evidence](../history/ws108/conformance.md); no new Python-wide formatter policy |

## WS109 native coverage / 2026-10-02

[承認の全文](ws109-native.md)。FreeBSD guestはnative compile/header/ELF/実libc・OSS/vtnet/loader、SSH+QMP PNGを検証。actual GPU/sync/seat/WiFiはユーザー準備の実機で別に記録。C全文/clang-format/style-check/manualのcoverageを維持、code変更後の最終conformanceはp005。現在のguest/OS/module checksは結果をPhaseに追記する、mock/nativebuildをhardware結果としない。

## WS109 native build/service/shared-mechanism coverage

Native build `gmake -j16 -f userland/desktop/keiland-freebsd.mk libraries` and DESTDIR/header-dependencies
use baseClang19.1.7/native headers/libs, no target libc. q553/q554/q555/q556 evidence and probes in
plan/ws109/tests cover actual SCM_RIGHTS/OSS/link/carrier/nativeUnix/publicUI/PDF/headlessVk; mockWPA
is wire-only. NativeGPU/WiFi/seat and full3OS/fullWS source manual conformance remain acceptance gates.
Cstandard still full coding-style.md; clang-format19 plus style-check/manual ownership/semantic review.
Boundary check.sh recognizes nativefreebsd OS roots, rejects OSmacro outside zwl-evdev.h and ioctl
outside OS modules, except the five shared native evdev metadata/clock request families in exactly
wayland/evdev/input-evdev.c. Its header selector owns record/constants ABI, OS seat owns leases.
No new operation or C relocation-style exception; unknown request families still fail the checker.

## WS109 acceptance revision / 2026-10-02

User waives physical-machine tests; actual native FreeBSD QEMU Venus replaces that gate. Earlier hardware-check descriptions are superseded for WS109. Full C/manual review and affected native/Linux/zedBSD checks remain required; no host-only or lavapipe substitute. [Decision](ws109-native.md).

## WS109 final native GPU coverage / 2026-10-02

[Retainednativeprobes](../tools/keiland-freebsd/README.md): systemheader/object/privateELF audit; realpipe/closedfd/output/borrowedownership and realGPU zeroaccesssync boundary; properxdgtoplevel/Vulkan positiveframes; actualVT/nativeleases/reacquisition/QMPUSBinput; sevenmainapps/fileopens and kernel→Wayland→PTYkeyboard. Manual fullC covers semanticparagraphs/function/declorder/ownership, formatter19.1.7 and stylechecker remain auxiliary (permitted forwardcleanup gotos are candidates, not violations). q566 unchangedsource/Linux/zedBSD receipts retained by hashes, q569..q572 nativeonly additions finalreviewed. Userselectedi915 replaces missingnativeVenusroute; physicalWiFi waived, WPApeer staysmockwire evidence. Source-only testmoves retain fullreview/evidence, WSspecifictests removed at completion perAGENTS.

## 2026-10-02 / ws109-physical-build-install

最新ユーザー: awe@10.0.30.3 ~/zedBSD実機の全操作を事前承認、make keiland-freebsd / sudo make keiland-freebsd-install / /opt/keiland直接起動を希望。最後のGUI受け入れはユーザーの実操作確認。先の実機waiverをこの受け入れについて置換、q572 evidenceは歴史として保存。SSH hostkey変更はユーザーが新ED25519指紋を確認済み、task専用known_hostsで接続。WIP commitに続く開発host pushとFreeBSD pullも追加指示で今回承認（従前push禁止のscope例外）。非force pushのみ、remote人間変更を保つ。native compiler/base libcと既存packages、seatd/video設定を利用、make toolchain不要。FreeBSD GDMはportのWayland制限説明後ユーザーが撤回、対象外。全newsourceの全文規約確認をp007で実行。Issue/Project/comment公開承認とは区別しoutboxを保つ。
