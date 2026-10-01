<!-- awesome-plan project=zedbsd record=ws107p003 -->

# ws107p003: 独立 API と component 実装の不備を直す

Status: cleared
Disposition: normal
Parent: [WS107](/home/awe/zedBSD-claude1/plan/ws107/ws.md)
Queue / Attempt: q543 / q543-i01

## 目的・範囲

p001 で確定した view/描画/入力/所有の問題だけを修正。標準 Vulkan の独立 client と shell 入力 adapter を検証。

## 完了条件

B3/B4。未発見の欠陥を推測して大規模 rewrite しない。重大な契約変更は実行前に計画へ反映する。

## 前提・未決・実行手順

依存: p002。WS の scope と acceptance、設計の未決を確認する。
技術的な細部は委任範囲で決める。対象/受け入れ/外部契約を変える結果は実装前に計画と承認範囲へ反映する。
調査→変更表と手順の確定→有限 Queue の承認→実装→指定検証→結果・WS・Master・Queue の照合。
最初の p001 は調査の案（1 session / 最大60分、満たせない点と再開条件を残す）。後続の timebox/command は設計後に選定する。

## 適用規則・影響する部品・検証

[WS の制約/部品/受け入れ](/home/awe/zedBSD-claude1/plan/ws107/ws.md)、[Guardrail](/home/awe/zedBSD-claude1/plan/guardrail.md)、
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

## 2026-10-02 / ws107-q541-design

[確定設計](/home/awe/zedBSD-claude1/plan/ws107/design.md)と[179file台帳](/home/awe/zedBSD-claude1/plan/history/ws107/inventory.json)を採用。viewの入力/描画境界とnavigationの失敗時transaction/async history、14style候補を修正。callbackは同じview mutation/destroyを外側call後へ延期するユーザー決定を公開仕様へ反映。public ABI v2保持、標準Vulkan動的第2clientでB3/B4を検証。依存p002。

callback判断の出典: このchat、2026-10-02回答「同じ view の変更・破棄は callback 後に行う契約にする」。scope/修正項目は設計に限定。GitHub Phase/WS delivery outbox pending。

2026-10-02 / ws107-style-decision: ユーザー「移動部分の既存スタイル維持を認める」。[限定例外](/home/awe/zedBSD-claude1/plan/standards/ws107-relocation.md)で不変moveのstyleを維持、品質修正/新試験/14候補をC全文で確認。受け入れと依存は保持、全hash/diff/boundary reviewは省略しない。

2026-10-02 / ws107-q543-style-review: 14候補を全文で点検、handler4＋loader loop1を修正、残9は既存critical section空行のscanner false positive（C全文§5）。同期挙動を変えず根拠をcomponent-resultへ。移動限定例外とは別の実rule判定であり、違反を免除しない。

## 結果 / q543-i01 / 2026-10-01T15:46:35.289346+00:00

cleared。有限component修正/API v2のcallback・借用契約、2view/入力/timer/所有/async history/late allocation rollback/実標準Vulkan/caller record-fence-releaseを検証。最終ASan/UBSan client83checks PASS、target/host warning0、既存host-view59/0。14style候補を全文判定（5修正、9critical section false positive）。plan/ws107/component-result.md。

Event: ws107-q543-cleared。Phase結果とclosure意図をlocal保存、GitHub comment/closeはdeferred、remote close未確認。
