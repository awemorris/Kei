<!-- awesome-plan project=zedbsd record=ws109p005 -->

# ws109p005: 全文規約・主な app と3 OS の最終回帰

Status: uncleared
Disposition: normal
Parent: [WS109](/home/awe/zedBSD-claude1/plan/ws109/ws.md)
Queue / Attempt: q566 / q566-i01

## 目的・範囲

全 port source を C 全文/OS 境界で review。F1〜F5、実表示/session/サービスと文書を照合し、Linux/zedBSD の影響範囲を再検証。

## 完了条件

F1〜F5。FreeBSD build のみを移植完了としない。未実施の GPU/実機/OS version を記録。

## 前提・未決・実行手順

依存: p003、p004。WS の scope と acceptance、設計の未決を確認する。
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

full C/OS rule reviewとaffectedLinux/zedBSD/nativeFreeBSD回帰を行う。F1実機model/driver、F3/F4実機の未実施はWS final acceptanceに残す。WS完了を判定する前に実機結果を確認。 [origin](/home/awe/zedBSD-claude1/plan/ws109/phase001/phase.md)・[WS summary](/home/awe/zedBSD-claude1/plan/ws109/ws.md)・[scope](/home/awe/zedBSD-claude1/plan/standards/ws109-native.md)。foreign Phase own検証/再開条件を更新、remote comment pending。

## q551 design reconciliation / ws109-q551-native-environment

Prerequisites include p002 final L2 integration plus p003/p004 implementation and retained physical gates. Full all-WSsource standards/3OS affected regressions; no VM-only WS completion. Reason: actual native dependencies and backend/link ordering inspected in p001. [Origin](/home/awe/zedBSD-claude1/plan/ws109/phase001/phase.md), [WS](/home/awe/zedBSD-claude1/plan/ws109/ws.md), [native design](/home/awe/zedBSD-claude1/plan/history/ws109/q551/environment.md). Own revised verification/resume condition saved; remote structural comment pending.

## q566 early final-conformance subset / native integration output available

Maximum60min/onePhase. Prerequisite p002 complete F2/q565 verified; consume actual q554/q555
OSS/network and q557/q559 shared/native seat implementation outputs, not whole p003/p004
physical acceptance. Independent final standards/regression/docs can execute before those real
GPU/WiFi gates; whole p005/F5 still requires them and successful actual main-app operations.
Full WS final changed-source inventory from WS108 records commit681b1566, native/full C authoritative
manual review and checked formatter/style automation. Resolve retained common libwayland/wire.c
and files/places.c full-file violations without unrelated mass formatting; preserve evaluation,
protocol/fd/ownership/order and public semantics. Fix necessary in-scope error handling when
standards require individually checked fallible calls; meaningful contract test accompanies any
observable correction. Native final build/install/affected contracts and Linux/zedBSD affected
build/regressions -j16 warning0, final boot-test PNG; scoped commands, artifacts and limits.
Add concrete native build/service/launch/physical checklist documentation. No guest GUI success,
GPU fence or WiFi hardware result fabricated. No new public API/architecture/HAL/toolchain policy.
If unknown prerequisite or review exceeds finite bounds, preserve partial outcomes and end uncleared.
Whole WS remains incomplete until its own hardware/main-app gates and final revalidation satisfied.
Source paths: all WS109 changed own source/build memberships for review; edits limited to in-scope
conformance corrections, native operational documentation and owning tests/evidence. Prior source
movement has no style exception. No make check/push/publication/shared cleanup/host installation.
Structural event ws109-q566: own p005 dependency distinguishes verified source outputs from remaining
whole physical acceptance; WS projection updated now, unchanged p003/p004 criteria not withdrawn.

## 2026-10-02 / ws109-20261002-qemu-venus-acceptance

Current user, this chat async reply: 「実機検証は不要です。qemuでVenusが使えればclearとします。」
This explicitly replaces the earlier user-provided-machine gate. Physical GPU/display/WiFi
acceptance is waived for WS109; do not request hardware or reintroduce those gates. Required
replacement evidence is actual FreeBSD QEMU Venus usage; mere host support, headless lavapipe or
Linux/zedBSD Venus does not establish that evidence. Native backend/build/standards and affected
regression obligations remain. p004 hardware radio operations become waived, not falsely tested;
native audio/wired and native radio ABI/refusal/WPA wire evidence remains classified accurately.
p003/F3 and p005/F5 replace physical display/main-app checks with owned FreeBSD QEMU Venus-backed
checks. If the native guest stack lacks a required Venus driver, investigate a bounded actual
capability chain and expose the remaining platform/scope choice; kernel/driver port is still outside
WS109's agreed scope. Current q566 standards/regression/docs subset stays authorized; Venus
configuration/implementation is selected separately after q566. No automatic WS/Phase clearance.
Origin user decision reconciled to WS/all changed Phase own criteria, Guardrail/scoped standard,
Queue supplement and docs; remote decision/structural events pending publication.

## Result / q566-i01 / 2026-10-02T00:22:28.921774+00:00

Queue item cleared /whole Phase uncleared。Final changed-source standards, wire/Places fixes, native docs and affected native/Linux/zedBSD checks PASS. [result](/home/awe/zedBSD-claude1/plan/history/ws109/q566/result.md). Physical tests waived; whole p005/WS pending actual FreeBSD QEMU Venus.

Event ws109-q566-cleared: local evidence/outcome saved; remote comment (no Phase close) pending.
