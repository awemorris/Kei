<!-- awesome-plan project=zedbsd record=ws107p002 -->

# ws107p002: エンジンの所属と build/test の参照を移す

Status: cleared
Disposition: normal
Parent: [WS107](/home/awe/zedBSD-claude1/plan/ws107/ws.md)
Queue / Attempt: q542 / q542-i01

## 目的・範囲

p001 の engine source/assets を libbrowser へ移し、Makefile/依存/生成器/現役 runner の経路を更新。main と shell は library の public API を使う。

## 完了条件

B1/B2 を満たし、semantic-preserving 部分の clean build が warning 0。

## 前提・未決・実行手順

依存: p001。WS の scope と acceptance、設計の未決を確認する。
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

[確定設計](/home/awe/zedBSD-claude1/plan/ws107/design.md)と[179file台帳](/home/awe/zedBSD-claude1/plan/history/ws107/inventory.json)を採用。165 engine filesとbuild/runner/生成器locatorだけをsemantic-preserving移動。private/app変数を分離、B1/B2とfresh engine/forced shell buildを確認。依存p001の台帳と設計。

callback判断の出典: このchat、2026-10-02回答「同じ view の変更・破棄は callback 後に行う契約にする」。scope/修正項目は設計に限定。GitHub Phase/WS delivery outbox pending。

## 結果 / q542-i01 / 2026-10-01T15:14:46.945762+00:00

cleared。engine165files（133C）のmove/hash/mode確認、うち生成器4filesだけlocator更新、表/shader binary未再生成。libbrowser/appのsource変数とprivate includeを分離、registry/exports/API v2/install保存。target libbrowser133fresh source＋browser7forced source/probe build exit0/warning0。host clean build exit0/warning0、public-only browser-probeのDT_NEEDEDはlibbrowser.so/libcのみ、libraryはhost標準Vulkan/libm/libcのみ（targetもWayland無し）。list-sources140C、runner syntax/diff-check PASS。

Event: ws107-q542-cleared。Phase結果とclosure意図をlocal保存、GitHub comment/closeはdeferred、remote close未確認。
