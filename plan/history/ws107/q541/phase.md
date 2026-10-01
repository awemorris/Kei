<!-- awesome-plan project=zedbsd record=ws107p001 -->

# ws107p001: engine/shell・API・品質の点検と設計

Status: cleared
Disposition: normal
Parent: [WS107](../ws.md)
Queue / Attempt: q541 / q541-i01

## 目的・範囲

tracked engine ファイルと参照を列挙し、現 public API v2、描画と view の所有、入力 adapter、callback と複数 view の契約を実装と突き合わせる。

## 完了条件

移動表・残す表・問題一覧・修正範囲と独立 client の検証手順を固定。public ABI の変更が必要なら影響と version 方針を示す。

## 前提・未決・実行手順

依存: WS074 の source（context）。WS の scope と acceptance、設計の未決を確認する。
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

## 2026-10-02 / ws107-q541-design

[確定設計](../design.md)と[179file台帳](../inventory.json)を採用。q541調査: engine165file/133C、残す14file、現source/link閉包、viewの有限修正6項とhost/target検証を固定。public ABI v2の変更不要。

callback判断の出典: このchat、2026-10-02回答「同じ view の変更・破棄は callback 後に行う契約にする」。scope/修正項目は設計に限定。GitHub Phase/WS delivery outbox pending。

## 結果 / q541-i01 / 2026-10-01T15:09:38.137819+00:00

cleared。179file移動台帳（engine165/133C、残す14）、API v2/標準Vulkan/Wayland無し境界と有限quality修正、target/host/client検証を確定。callbackの同一view変更・破棄をcall後へ延期するユーザー判断を保存。style14候補はp003で全文適合。plan/ws107/design.mdとinventory.json、全変更Phase/WSイベント。

Event: ws107-q541-cleared。Phase結果とclosure意図をlocal保存、GitHub comment/closeはdeferred、remote close未確認。
