<!-- awesome-plan project=zedbsd record=ws109p004 -->

# ws109p004: audio・network・WiFi の FreeBSD backend

Status: planning
Disposition: normal
Parent: [WS109](../ws.md)
Queue / Attempt: なし（未承認）

## 目的・範囲

p001 で選んだ native audio/mixer、interface/link 状態、WiFi 管理を libkeiland の backend に実装。共通 WPA wire が適用できる部分は再利用。

## 完了条件

F4。PCM 再生を含めるかは p001 で確定し、WS105 の音量 backend と取り違えない。

## 前提・未決・実行手順

依存: p002。WS の scope と acceptance、設計の未決を確認する。
技術的な細部は委任範囲で決める。対象/受け入れ/外部契約を変える結果は実装前に計画と承認範囲へ反映する。
調査→変更表と手順の確定→有限 Queue の承認→実装→指定検証→結果・WS・Master・Queue の照合。
最初の p001 は調査の案（1 session / 最大60分、満たせない点と再開条件を残す）。後続の timebox/command は設計後に選定する。

## 適用規則・影響する部品・検証

[WS の制約/部品/受け入れ](../ws.md)、[Guardrail](../../guardrail.md)、
[C 規約全文](../../coding-style.md)、[自動化](../../standards/automation.md)を適用。
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

OSS mixer/vtnet/native networkとsupplicant backend実装を先行可。mockwireの結果はcontractだけ。realWiFi scan/connect/disconnect/configのcriteriaはユーザー準備の実機でverify、partial実装Queueと全F4を分ける。 [origin](../phase001/phase.md)・[WS summary](../ws.md)・[scope](../../standards/ws109-native.md)。foreign Phase own検証/再開条件を更新、remote comment pending。

## q551 design reconciliation / ws109-q551-native-environment

Prerequisite is verified p002 L1 headers/libraries, not full F2 integration. Native OSS/AF_LINK/WPA backend implemented ahead of final integration, actual audio/wired VM checks and mocked WPA distinguished from physical WiFi acceptance. Reason: actual native dependencies and backend/link ordering inspected in p001. [Origin](../phase001/phase.md), [WS](../ws.md), [native design](../../history/ws109/q551/environment.md). Own revised verification/resume condition saved; remote structural comment pending.
