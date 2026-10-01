<!-- awesome-plan project=zedbsd record=queue -->

# Queue

Status: active
Active Queue: q538
Last finished Queue: q537

## q538

- Purpose: 規約の全文の見直し、境界の確かめの拡張、回帰、install の文書
- Focus: fg012 / WS105 の既存 Linux の受け入れを満たす。
- Timebox: 60 分。既知の build・必須検証は最大 150 分、未知の問題の調査は 30 分で結果と再開条件を記録する。
- Approval: current user, 2026-10-01「ws105の完了をゴールにして、自走をお願いします。」。既存 WS105 p001〜p011 の範囲を依存順の 1 Phase Queue で実装・検証・記録する。commit は WIP、push / GitHub 公開は行わない。
- Exact approved scope: [ws105-p011](ws105/phase011/phase.md) 全範囲、開始前 [snapshot](history/ws105/q538/scope.md) SHA256 `cc64af7e3f6a6f579e373225a171ba23b6020be49b8b3484c00d73ec1f642c35`。
- Executor: Codex Q1。前 Queue finished、並行 executor なしを確認。
- Applicable rules: AGENTS.md、Guardrail、coding-style.md 全文、WS105 D1〜D25、design.md の指定節。
- Prerequisites: Phase の依存と既存出力を実装・証拠で確認。material scope / criteria の変更・取消なし。
- Started UTC: 2026-10-01T11:56:09.585238+00:00

| Attempt | Phase | Status | Dependency | Selection reason |
| --- | --- | --- | --- | --- |
| q538-i01 | [ws105-p011](ws105/phase011/phase.md) | in-progress | p001〜p010（context） | WS105 の既存依存順 |

Dependency graph: Phase の依存（context）→ q538-i01/ws105-p011。他の Phase は次の Queue で選定する。

## Upcoming Work Outlook

WS105 の依存を満たす次の Phase。新しい product / risk 判断はユーザーの決定が前提。既存 WS105 の完了までの自走指示で選定・実行する。
GitHub への publication は deferred、outbox に保持する。
