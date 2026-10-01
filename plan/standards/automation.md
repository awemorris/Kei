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
| engine/source 所有と shell のみの app | tracked inventory、expanded Makefile source、private include の dependency | 現 engine source は browser に残る。WS107 p001/p002 で台帳/移動、p004 で最終確認。新 checker は未実装 |
| libbrowser の Wayland 使用禁止 | public header standalone compile、include/link closure、ELF NEEDED/undefined/export | public header と直接名の走査だけ調査済み。完全な依存/実行検証は未実施 |
| 標準 Vulkan と抽象入力 | Wayland 無しの第2 client の Vulkan 描画、shell adapter/複数 view の契約検証 | API v2 に既存の抽象がある。lifetime/failure/callback/reentrancy は full/manual review と実 behavior の確認が要る |

WS106 では source 移動後の既存 Makefile/source checker の locator を更新する。WS108/109 の distro/FreeBSD checks は設計中、
WS105 の Linux checks の合格を新 target の合格に転用しない。各 WS の near-final Phase で全文規約/実環境/tool version と限界を記録する。

## WS106 relocation coverage (2026-10-01)

[User-authorized scoped exception](ws106-relocation.md): preserve existing implementation style for path-only relocation. Validate all move hashes/modes and allowed include/path diffs; compare normalized style-check baseline (59 C,1300 candidates). New C/semantic changes remain subject to the full standard. Review all WS source/config/runner diffs, registry/default/dependency/install contracts and final build/boot. No mass formatting.

WS107: [限定例外](ws107-relocation.md)の不変moveをhash/mode/path-only diffでreview。変更componentと新試験はC全文/edited-range clang-format/manual review、既知14候補を修正し全133C style-checkを比較。
