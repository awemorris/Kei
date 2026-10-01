<!-- awesome-plan project=zedbsd record=ws107 -->

# WS107: libbrowser の source 所有と独立コンポーネントの整備

<!-- awesome-plan-current:start -->
Status: incomplete
Primary Milestone: MG006
Related Milestones: MG002
Parent: [Master](../master.md)
Queue: なし（q541 finished）
Resume point: p001 cleared、p002の既存承認scopeと実outputを確認。
<!-- awesome-plan-current:end -->

## 目標・決定の出典

HTML/JS/描画の実装を libbrowser に所有させ、browser を library のコンポーネントを窓とタブに包むアプリにする。

2026-10-01 ユーザーの [レビューコメント](../reviews/2026-10-01-review.md)を根拠に計画。
Primary MG006 にこの目標の成果を提供。Related MG002 は技術/配布/検証の supporting 出力。
WS completion と milestone 全体の acceptance は別に確認する。

## 範囲

[設計・現状対比](design.md)、[browser の境界の全文規則](../standards/browser-component.md)に従う。
エンジンの source/header/表/shader/生成器を `userland/desktop/browser/` から `userland/desktop/libbrowser/` に移す。
`main.c` と `shell/`、開始頁等のアプリデータは browser に残す。ユーザーの「source を移す」はこの責務分離として記録する。
browser は既に libbrowser.so に link している。source の所属と API/所有/失敗処理の品質を直す必要がある。
libbrowser は標準 Vulkan を使えるが Wayland を使用しない。shell が入力を抽象化して public API に渡す。
品質点検は view の lifecycle、描画先/fence の所有、複数 view の独立、入力/callback/reentrancy の契約を対象にする。
Acid3/CSS2 の網羅的な互換性向上は WS074 の別の目標。全面的な engine 書換えや Qt/GTK への組込みは加えない。

## WS 自身の完了条件

- B1: engine の全 tracked source/asset が libbrowser に属し、browser には app shell とそのデータが残る。
- B2: browser が libbrowser.so に動的 link し、engine の再コンパイル/内部 header への依存が無い。
- B3: libbrowser の公開/内部 source・link が Wayland に依存せず、標準 Vulkan と抽象入力だけで独立した第2の client が利用できる。
- B4: 2 view の独立・入力・callback・create/destroy・resize・描画先解放/失敗時の所有を検証し、点検で見つけた in-scope の不備を修正。
- B5: 既存 Acid2/headless/CLI/shell の必要な回帰、warning 0 の build、最終 boot と全文規約を確認。

## 依存・所有

WS074 の現行 engine を引き継ぐ。p100→p101 の順・目標を保つ。WS074 の今後の source 編集は新しい境界規則を適用し、同一 tree の移行と並行しない。WS106 の browser-probe 移動は実際の locator を解決して試験に用いる。

## Phase 表（後続は設計案）

| ID / Phase | 目的 | Goal | Status | 依存 |
| --- | --- | --- | --- | --- |
| [ws107p001](phase001/phase.md) | engine/shell・API・品質の点検と設計 | 移動表・残す表・問題一覧・修正範囲と独立 client の検証手順を固定。public ABI の変更が必要なら影響と version 方針を示す。 | cleared | WS074 の source（context） |
| [ws107p002](phase002/phase.md) | エンジンの所属と build/test の参照を移す | B1/B2 を満たし、semantic-preserving 部分の clean build が warning 0。 | planning | p001 |
| [ws107p003](phase003/phase.md) | 独立 API と component 実装の不備を直す | B3/B4。未発見の欠陥を推測して大規模 rewrite しない。重大な契約変更は実行前に計画へ反映する。 | planning | p002 |
| [ws107p004](phase004/phase.md) | 全文規約と shell/engine の最終回帰 | B1〜B5 を満たす。CPU の成功を Vulkan の検証に代用しない。 | planning | p003 |


依存は表の prerequisite → dependent。context は選定された作業ではない。
後続 Phase の detail は p001 の確定設計から作る。表/実 record/Queue を一緒に整える。

## 制約・標準・検証の扱い

[Guardrail](../guardrail.md)、[AGENTS.md](../../AGENTS.md)、[C 規約全文](../coding-style.md)、
[自動化の対応](../standards/automation.md)を適用する。簡約版は無い。コード生成前に全文の該当節を読み、
最後の conformance Phase では本 WS の全 source 変更を全文でレビューする。
clang-format 19 / style-check は補助。無関係な一括整形、`make check`、`.internal/` の参照は行わない。
意味を変えない移動は warning 0 の build と最後の boot を関門にする。API・platform・packaging の変更には意味のある契約検証を追加する。
image build は直列。zedBSD の起動は boot-test.sh と PNG、console/serial log を起動の証拠にしない。
commit は `WIP`、push なし。GitHub 公開・Issue/Project 更新は現在 deferred、local cache/outbox に記録する。

## 現状・再開

2026-10-01、source `01c754a0` を調査して計画を新設。実装・移動・CI job 追加は未実施。
Queue は無し。p001 の計画を確認して有限 Queue を選定する。後続 Phase は案で、調査結果によって詳細化する。
既存 q538 は finished、WS105 の完了範囲を拡張しない。

## イベント

2026-10-01 / review-20261001-planning: ユーザーのレビューコメントから WS を新設。
範囲・受け入れ・Phase 案を保存、Master / Outlook と照合した。新規実装の Queue 承認は未取得。
[決定の出典と関連 WS](../reviews/2026-10-01-review.md)。公開時にはこのイベントを WS に届ける（現在 outbox 保留）。

2026-10-02 / ws107-q541-design: p001〜p004を[確定設計](design.md)へ詳細化、engine165/残す14file、有限のquality修正/第2client/回帰を固定。callbackはユーザーの同一view変更/破棄を外側call後へ延期する判断を採用。API v2/既存B1〜B5/依存順を保持、WS074の互換性目標を追加しない。

2026-10-01T15:09:38.139159+00:00 / ws107-q541-cleared: p001 cleared。179file移動台帳（engine165/133C、残す14）、API v2/標準Vulkan/Wayland無し境界と有限quality修正、target/host/client検証を確定。callbackの同一view変更・破棄をcall後へ延期するユーザー判断を保存。style14候補はp003で全文適合。plan/ws107/design.mdとinventory.json、全変更Phase/WSイベント。
