<!-- awesome-plan project=zedbsd record=ws106 -->

# WS106: テスト用アプリを userland/tests/ に集約

<!-- awesome-plan-current:start -->
Status: planning
Primary Milestone: MG001
Related Milestones: MG006
Parent: [Master](../master.md)
Queue: なし（新規実装未承認）
Resume point: p001 の調査・設計を選定する前に scope と検証環境を確定。
<!-- awesome-plan-current:end -->

## 目標・決定の出典

base・desktop のテスト用アプリと指定された見本を userland/tests/ に移し、既存の選択・実行・デモの振る舞いを保つ。

2026-10-01 ユーザーの [レビューコメント](../reviews/2026-10-01-review.md)を根拠に計画。
Primary MG001 にこの目標の成果を提供。Related MG006 は技術/配布/検証の supporting 出力。
WS completion と milestone 全体の acceptance は別に確認する。

## 範囲

[対象表](inventory.md)の 30 件（base 21、desktop 9）を対象にする。gpudemo は実行 binary ではなくデモデータ package。
移動先は `userland/tests/<name>/`。`userland/base/test/` は POSIX の test utility なので残す。
host の検証 fixture / plan の履歴 / production app / library はこの移動に含めない。
source、header、Makefile/Makefile.linux、shader、モデル等の付属データを package ごとに移す。
共通 package.mk と grouping、package registry/dependency の経路、config・test runner・Linux include の参照を整える。
元の package 名、config の選択名、install 先、既定の選択状態、デモ S13 を保つ。category の変更は p001 で定義する。
`ime-probe` は WS095 の人間作業と重なるので、実際の移動前に所有・作業停止を確認する。

## WS 自身の完了条件

- T1: 30 件の全ファイルと移動元→先・参照変更を台帳化し、対象外を明記。
- T2: 二重登録や古い source 参照が無く、既存 package/config/install の契約を保つ。
- T3: amd64 zedBSD と既存 Linux 対応 5 app package の clean build/install が warning 0。gpudemo/model/shader のデータも一致。
- T4: 既存の移動に関係する runner が新経路を利用でき、最後の zedBSD boot PNG と全文規約レビューを記録。

## 依存・所有

WS105 の既存 Linux 出力を利用。WS095 の ime-probe は人間の作業と調整。WS107 の browser-probe は public API を使うためエンジン移動と実装依存は無いが、共通 runner 編集は直列。WS108 は最終のテスト配置を取り込む。

## Phase 表（後続は設計案）

| ID / Phase | 目的 | Goal | Status | 依存 |
| --- | --- | --- | --- | --- |
| [ws106p001](phase001/phase.md) | 対象・参照・build 契約を確定 | 台帳と移動手順、実際の build/config/check command を固定。所有が未調整なら ime-probe の実行を選定しない。 | planning | なし |
| [ws106p002](phase002/phase.md) | source・データと参照を移動 | T1/T2 を満たす。source とデータの move 前後の hash と差分を記録する。 | planning | p001 |
| [ws106p003](phase003/phase.md) | 全文規約・build/install・最終 boot | T1〜T4 と全 Phase の成果を照合。未実施の機種・アプリ実行は区別して記録する。 | planning | p002 |


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
