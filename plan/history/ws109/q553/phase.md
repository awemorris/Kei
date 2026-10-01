<!-- awesome-plan project=zedbsd record=ws109p002 -->

# ws109p002: native build・library と system Vulkan chain

Status: uncleared
Disposition: normal
Parent: [WS109](/home/awe/zedBSD-claude1/plan/ws109/ws.md)
Queue / Attempt: q553 / q553-i01

## 目的・範囲

p001 の確定設計で package ごとの Makefile.freebsd と独立 build/install を実装。共通 source と OS backend 選択を明示。

## 完了条件

F2。glibc 固有の loader binding に頼らないことを実際の FreeBSD で検証。

## 前提・未決・実行手順

依存: p001。WS の scope と acceptance、設計の未決を確認する。
技術的な細部は委任範囲で決める。対象/受け入れ/外部契約を変える結果は実装前に計画と承認範囲へ反映する。
調査→変更表と手順の確定→有限 Queue の承認→実装→指定検証→結果・WS・Master・Queue の照合。
最初の p001 は調査の案（1 session / 最大60分、満たせない点と再開条件を残す）。後続の timebox/command は設計後に選定する。

## 適用規則・影響する部品・検証

[WS の制約/部品/受け入れ](/home/awe/zedBSD-claude1/plan/ws109/ws.md)、[Guardrail](/home/awe/zedBSD-claude1/plan/guardrail.md)、
[C 規約全文](/home/awe/zedBSD-claude1/plan/coding-style.md)、[自動化](/home/awe/zedBSD-claude1/plan/standards/automation.md)を適用。
新規/変更 C は全文該当節を読み、clang-format-19 と style-check の限界を補う。
最終 conformance は全 WS の source を全文で review。build warning 0、必要な契約検証、diff-check を記録する。
具体的な build/config/tool version と script は p001 の結果で固定する。`make check` は禁止。
zedBSD の起動は boot-test.sh の PNG。Linux の既存検証は WS105 の手順と許可範囲、FreeBSD は検証環境の確定が必要。

## 証拠・結果・再開条件

未実行。command/result/commit/artifact/skipped checks はまだ無い。計画の作成を clearance としない。
再開: prerequisite の実 output と変更の所有、scope snapshot、Queue 承認を確認する。

## イベント

2026-10-01 / review-20261001-planning: 新設した Phase 案。親 WS の目標への寄与と依存を記録。実装の選定は未実施。
GitHub の Phase 作成/comment/Project の projection は公開保留、local outbox に保持する。

## 2026-10-02 / ws109-user-decisions-20261002 / このPhaseへの反映

ユーザー「drm-kmodを利用OKです。FreeBSDにも例外を適用します。FreeBSD実機は用意しておくので、作業を進めておいてください。」（2026-10-02 JST、このchat）。D1:既存FreeBSD drm-kmod利用可、GPL-free systemstack条件をこの範囲で置換。Keiland sourceの寛容license/外部実装を取り込まない境界は維持。D2:WS109専用FreeBSD QEMU guestのloopback SSH/QMP PNG検証を承認。D3:実機はユーザーが準備、入手前にnative build/backend実装を進める。実GPU/WiFi結果は将来の実機関門に残し、mock/QEMUbuildで代替しない。

p001のfixed software/ABI/license/guest outputをprerequisiteとしてnative library/build/install/loaderの実検証を先行。GPU hardware availabilityはこのbuild scopeの前提にしない。全app linkにp003/p004実backendが必要ならstubを作らず対象を部分選定し後で統合する。 [origin](/home/awe/zedBSD-claude1/plan/ws109/phase001/phase.md)・[WS summary](/home/awe/zedBSD-claude1/plan/ws109/ws.md)・[scope](/home/awe/zedBSD-claude1/plan/standards/ws109-native.md)。foreign Phase own検証/再開条件を更新、remote comment pending。

## q551 design reconciliation / ws109-q551-native-environment

p002 is staged: L1 foundation libraries first, L2 real libkeiland/compositor/apps after p003/p004 implementation output. Retain full F2 acceptance; L1 scoped item can clear with wholePhase uncleared until integration. Reason: actual native dependencies and backend/link ordering inspected in p001. [Origin](/home/awe/zedBSD-claude1/plan/ws109/phase001/phase.md), [WS](/home/awe/zedBSD-claude1/plan/ws109/ws.md), [native design](/home/awe/zedBSD-claude1/plan/history/ws109/q551/environment.md). Own revised verification/resume condition saved; remote structural comment pending.

## q552 L1 foundation / exact partial scope

Delegated WS109 native implementation under user instruction; maximum60min/1Phase. Implement standalone `userland/desktop/keiland-freebsd.mk` + native package Makefile.freebsd for base codecs, digest static library, private Wayland-client, TrueType and own Vulkan compatibility library. Native flags/paths/header staging/DESTDIR install/system-inclusive dependencies and print-sources. Build only actual L1 libraries; reserve compositor/libkeiland/apps for L2 after real p003/p004 outputs. No successful stubs/target toolchain changes. Native BSD install and DRM include layout; shared wsi-swapchain calls small Linux/FreeBSD sync modules, preserving errno/fd/fallback/public behavior. Fine OS UAPI record checked against fixed upstream and native ioctl encoding. No framebuffer/backend availability claims from headless Vulkan.

Scope files: independent native makefiles listed above; `libvulkan-compat/{dma-sync.h,linux/sync-linux.c,freebsd/sync-freebsd.c,wsi-swapchain.c,Makefile.linux,Makefile.freebsd}`; Phase-specific native foundation contract probes and evidence. Reuse existing standard header/vk-chain Linux tests where portable; new tests only for meaningful native ABI/fd/syscall differences. Code-style full standard loaded, clang-format19 edited scope/style-check/manual; native build -j16 warning0, DESTDIR/header/ELF/loader chain buffer copy using real system loader+Mesa, Linux affected library/chain regression. zedBSD Vulkan builds different library, no shared sync module linked; final relevant boot in p005. Whole Phase F2 remains uncleared after L1 until all real application/backend integration. p003/p004 eligibility depends on verified L1 outputs. WIP commit, no push; records/outbox pending publication.

## Result / q552-i01 / 2026-10-01T19:26:37.866206+00:00

Queue item uncleared /whole Phase uncleared。L1独立native Makefile/52sources、OSsync wrappersと共有WSI変更を実装。Linux Vulkan library buildはwarning0/exit0。FreeBSD buildはlibwayland/wire.cの未計画CMSG_ALIGN依存で失敗、runtime/DESTDIR未実施。[失敗](/home/awe/zedBSD-claude1/plan/history/ws109/q552/l1-build.txt)。新attemptで標準CMSG_SPACE/CMSG_LENによる整列とnative/Linux fd受渡しをscopeに加え、残るnative buildを再検証する。

Event ws109-q552-uncleared: local evidence/outcome saved; remote comment (no Phase close) pending.

## q553 redesign after q552 / internal L1 correction

q552 failed at native libwayland/wire.c because CMSG_ALIGN is Linux's internal macro. Extend L1 changed component to common libwayland/wire.c: compute aligned record extent with standard CMSG_SPACE(cmsg_len - CMSG_LEN(0)) after validated length, preserving bounds/short final controls/fd ownership and ordered ancillary handling. Add actual native/Linux SCM_RIGHTS valid/truncated/fragmented controls contract checks. Other external interfaces/dependencies/acceptance unchanged. Resume native52source build and resolve ordinary native compilation differences within those already selected components; no scope expansion into unrelated libraries/apps or architecture. Same max60min. Retain q552 failed result/evidence; full p002 F2 integration remains uncleared after L1. Event ws109-q553-redesign saved after plan revision; remote origin comment pending, no changed foreign Phase scope.

## Result / q553-i01 / 2026-10-01T19:40:30.363310+00:00

Queue item cleared /whole Phase uncleared。L1 native52sources/sevenELF＋DESTDIR/publicheaders/nativeVulkan1MiBchain、actualfd/CLOEXEC/truncation/errno所有を検証。Linux library/chain/fullWaylandsuite回帰PASS。[result](/home/awe/zedBSD-claude1/plan/history/ws109/q553/result.md)。fullF2のlibkeiland/compositor/apps統合はp003/p004後のL2、全source規約/zedBSD/physical gatesはp005に保持。

Event ws109-q553-cleared: local evidence/outcome saved; remote comment (no Phase close) pending.
