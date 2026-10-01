<!-- awesome-plan project=zedbsd record=q530 -->

# Queue

Status: finished
Active Queue: なし
Last finished Queue: q530

## q530

- Purpose: libvulkan-compat (3): 画面の WSI（KMS、VK_KHR_display、VK_EXT_acquire_drm_display）
- Focus: fg012 / WS105 の既存 Linux の受け入れを満たす。
- Timebox: 60 分。既知の build・必須検証は最大 150 分、未知の問題の調査は 30 分で結果と再開条件を記録する。
- Approval: current user, 2026-10-01「ws105の完了をゴールにして、自走をお願いします。」。既存 WS105 p001〜p011 の範囲を依存順の 1 Phase Queue で実装・検証・記録する。commit は WIP、push / GitHub 公開は行わない。
- Exact approved scope: [ws105-p005](ws105/q530/phase.md) 全範囲、開始前 [snapshot](ws105/q530/scope.md) SHA256 `ec79189842a5e81bdd82c924691b6c53433814cf3432cd3c47c68d045dfeeeba`。
- Executor: Codex Q1。前 Queue finished、並行 executor なしを確認。
- Applicable rules: AGENTS.md、Guardrail、coding-style.md 全文、WS105 D1〜D25、design.md の指定節。
- Prerequisites: Phase の依存と既存出力を実装・証拠で確認。material scope / criteria の変更・取消なし。
- Started UTC: 2026-10-01T08:26:52.417954+00:00

| Attempt | Phase | Status | Dependency | Selection reason |
| --- | --- | --- | --- | --- |
| q530-i01 | [ws105-p005](ws105/q530/phase.md) | cleared | p004、p001（guest）（context） | WS105 の既存依存順 |

Dependency graph: Phase の依存（context）→ q530-i01/ws105-p005。他の Phase は次の Queue で選定する。

## Upcoming Work Outlook

WS105 の依存を満たす次の Phase。新しい product / risk 判断はユーザーの決定が前提。既存 WS105 の完了までの自走指示で選定・実行する。
GitHub への publication は deferred、outbox に保持する。

## Outcome

ws105-p005 **cleared**。

cleared（q530-i01）。KMS completionのpollを各100ms以下、総期限5秒にした。単発timeout/EINTRはretry、総期限未完了はSURFACE_LOST、EACCES/EPERMとrevoked fdはOUT_OF_DATEを維持した。productionの試験用switchは無し。

- 旧staged implementationにtest-only flip-delay.soで1回250ms遅延＋poll result0を与え、赤→緑の後にFAIL result=-1000001004を再現した。修正後のseat fd/direct両経路は同じinjection (`requested_ms=100 delayed_ms=250 result=0`) 後に青を表示しPASS/exit0。全6色PNG各4点一致、oldSwapchain破棄後も表示、console復元、guest stop。
- gcc14.2/clang19.1.7 build exit0 warning0、12ELF/source-sync/126header PASS（p006の既存Linuxprogramを含む）。style-check0、clangformat19、変更KMS節の全文規約を点検。全WSの最終規約はp011で実施。
- host DRM=none: chain1MiB/PASS、interposebindings0/optoutPASS、Wayland FIFO/fallback/resize/MAILBOX360frameの画素/extent/import/privatewait PASS。
- q528のvkdemoとrootVT切替の結果は保持。今回変更はその単発SETCRTCの経路に影響しない。logind/revoked fdの実動作は既存p009で確認。実機GPU/物理monitor/Valgrind未実施。
- [新旧ログ・PNG・manifest](../../history/ws105/q530/evidence/)。p005のclearanceを復旧し、p006の同じ受け入れを次のQueueで再開する。q529のunclearedは履歴として保持。


実装 commit `753b45a0fae9ba22d0ef6d0a7fa0f8a4698b951d`。Finished UTC: 2026-10-01T08:31:05.223429+00:00。次の Phase は WS105 の既存完了までの自走指示で選定。push / GitHub 公開は未実施。
