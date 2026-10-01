<!-- awesome-plan project=zedbsd record=q535 -->

# Queue

Status: finished
Active Queue: なし
Last finished Queue: q535

## q535

- Purpose: libvulkan-compat (3): 画面の WSI（KMS、VK_KHR_display、VK_EXT_acquire_drm_display）
- Focus: fg012 / WS105 の既存 Linux の受け入れを満たす。
- Timebox: 60 分。既知の build・必須検証は最大 150 分、未知の問題の調査は 30 分で結果と再開条件を記録する。
- Approval: current user, 2026-10-01「ws105の完了をゴールにして、自走をお願いします。」。既存 WS105 p001〜p011 の範囲を依存順の 1 Phase Queue で実装・検証・記録する。commit は WIP、push / GitHub 公開は行わない。
- Exact approved scope: [ws105-p005](ws105/q535/phase.md) 全範囲、開始前 [snapshot](ws105/q535/scope.md) SHA256 `0b7e7469c447fa5dfb9c7e1a71e1cd3ced85d06cc81f3bb7b16f2bd7328cde95`。
- Executor: Codex Q1。前 Queue finished、並行 executor なしを確認。
- Applicable rules: AGENTS.md、Guardrail、coding-style.md 全文、WS105 D1〜D25、design.md の指定節。
- Prerequisites: Phase の依存と既存出力を実装・証拠で確認。material scope / criteria の変更・取消なし。
- Started UTC: 2026-10-01T10:43:57.091712+00:00

| Attempt | Phase | Status | Dependency | Selection reason |
| --- | --- | --- | --- | --- |
| q535-i01 | [ws105-p005](ws105/q535/phase.md) | cleared | p004、p001（guest）（context） | WS105 の既存依存順 |

Dependency graph: Phase の依存（context）→ q535-i01/ws105-p005。他の Phase は次の Queue で選定する。

## Upcoming Work Outlook

WS105 の依存を満たす次の Phase。新しい product / risk 判断はユーザーの決定が前提。既存 WS105 の完了までの自走指示で選定・実行する。
GitHub への publication は deferred、outbox に保持する。

## Outcome

ws105-p005 **cleared**。

cleared（q535-i01、p005のlogind fd ownership修復）。sourceは今回のKMS WIP commit。caller supplied master fdはAUTH_MAGIC magic0でcurrent-masterを確かめ、SET_MASTER不要。非masterは拒否。libraryのdirect取得だけmaster_owned=1とし、release時のDROP_MASTERもその経路だけ。borrowed seat fdのdupはcloseし、callerのfd/master権限は保つ。compositorへのDRM ioctl追加なし、public API不変。

- gcc14.2 / clang19.1.7 warning0、26ELF（24production+2fixture）/ source-sync / 331header PASS。clangformat19 / kms.cと新fixture style-check0、changed KMS ownership scopeを全文規約でreview。host DRMnone chain1MiB・interpose0 / optout・Wayland FIFO/fallback/resize/MAILBOX360frame PASS。
- guestの既存display-probeはseat fd/directの両経路で赤・緑・青全6PNGの4点一致、oldSwapchainの置換と破棄後も表示、各exit0/PASS。gdm停止直後のtty1にはgettyが無くBIOS画面を復元したので、そのPNGも残し、getty@tty1を起動後に両経路を再確認、Linux login prompt復元PNGを目視。guest-only新fixture seat-fdはnonmaster拒否、両callerfileがlive、release後のcaller master維持PASS。
- source3400a098のlogind seatを既存contextとしてgdm自動loginのuser keiでKeiland wallpaper / system barを表示、PNGを目視・提示。q534のSET_MASTER EACCES / frames0から回復した。LD_PRELOAD observerはstaged installで除去、production経路。p009のapp/VT/LogOutはこれから同じ条件で再検証する。
- [証拠](../../history/ws105/q535/evidence/SHA256SUMS)。元build/ws105-p005-logind/とbuild/ws105-p009/kms-*を保持。実機未実施、host package追加0、toolchain / target source変更0。guestはp009再開のため稼働、gdm停止。q528/q530当時の結果とq534失敗は保持。GitHub未公開、pushなし。


実装 commit `5012d324296ed75de618a7975321e1dddaa9f14b`。Finished UTC: 2026-10-01T10:49:48.408582+00:00。次の Phase は WS105 の既存完了までの自走指示で選定。push / GitHub 公開は未実施。
