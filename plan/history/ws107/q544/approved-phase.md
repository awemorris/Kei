<!-- awesome-plan project=zedbsd record=ws107p004 -->

# ws107p004: 全文規約と shell/engine の最終回帰

Status: planning
Disposition: normal
Parent: [WS107](../ws.md)
Queue / Attempt: なし（未承認）

## 目的・範囲

全 WS source を C 全文と browser 境界全文でレビュー。ELF/public header/standalone client、関係する pinned regression と最後の boot を照合。

## 完了条件

B1〜B5 を満たす。CPU の成功を Vulkan の検証に代用しない。

## 前提・未決・実行手順

依存: p003。WS の scope と acceptance、設計の未決を確認する。
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

[確定設計](../design.md)と[179file台帳](../inventory.json)を採用。全165engine/14appと全変更consumerを全文reviewし、B1〜B5/ELF/header/client/build/関係回帰/boot PNGを確認。後日の意味変更があれば再検証、CPUをGPUの代用にしない。依存p003。

callback判断の出典: このchat、2026-10-02回答「同じ view の変更・破棄は callback 後に行う契約にする」。scope/修正項目は設計に限定。GitHub Phase/WS delivery outbox pending。

2026-10-02 / ws107-style-decision: ユーザー「移動部分の既存スタイル維持を認める」。[限定例外](../../standards/ws107-relocation.md)で不変moveのstyleを維持、品質修正/新試験/14候補をC全文で確認。受け入れと依存は保持、全hash/diff/boundary reviewは省略しない。
