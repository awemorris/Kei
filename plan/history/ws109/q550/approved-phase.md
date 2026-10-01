<!-- awesome-plan project=zedbsd record=ws109p001 -->

# ws109p001: FreeBSD15 の graphics/OS 契約と環境を調査

Status: planned
Disposition: normal
Parent: [WS109](../ws.md)
Queue / Attempt: なし（未承認）

## 目的・範囲

Linux headers/ioctl/loader/service を棚卸し、FreeBSD15 の実 headers/library/driver と照合する。graphics 本体を共有できる境界、seat/input、audio/network/WiFi backend、GPL 条件、固定 guest/実機の検証方法を設計。

## 完了条件

F1 と port の対応表/実現可能な F2〜F5 手順。Linux DMA_BUF sync と同等の能力が無ければ別方式の影響と選択をユーザーに提示してから dependent 実装を選定。

## 前提・未決・実行手順

依存: WS105 output（context）。WS の scope と acceptance、設計の未決を確認する。
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

## 2026-10-02 実行指示 / 選定scope

ユーザー「WS108の完了後、WS109の実行をお願いします。」。WS108 completed/q549 finishedを確認。q550はp001のみ、最大60分。実sourceと公式15.x release/header/driver/API/licenseを照合し、fixed amd64 imageとnative build、graphics/session/input/audio/network/WiFiの変更対応表および検証環境を具体化。OS image/hashの取得と未起動guest設定の準備を含む。production変更・driver install・guest起動は本scopeに含めない。必要な人間の判断を明示してdependent実装を止める。実WiFi環境の質問をこのchatで提出、回答待ち。FreeBSD起動にはAGENTSの検証例外を確定してから実行。
