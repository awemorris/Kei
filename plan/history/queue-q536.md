<!-- awesome-plan project=zedbsd record=q536 -->

# Queue

Status: finished
Active Queue: なし
Last finished Queue: q536

## q536

- Purpose: gdm と logind（seat-logind・最小の D-Bus・pause と resume・`keiland.desktop`）
- Focus: fg012 / WS105 の既存 Linux の受け入れを満たす。
- Timebox: 60 分。既知の build・必須検証は最大 150 分、未知の問題の調査は 30 分で結果と再開条件を記録する。
- Approval: current user, 2026-10-01「ws105の完了をゴールにして、自走をお願いします。」。既存 WS105 p001〜p011 の範囲を依存順の 1 Phase Queue で実装・検証・記録する。commit は WIP、push / GitHub 公開は行わない。
- Exact approved scope: [ws105-p009](ws105/q536/phase.md) 全範囲、開始前 [snapshot](ws105/q536/scope.md) SHA256 `76ec4bc9d6a301432cc7dfffb4da7d94597bf9b3335a1107e4ab0b8a24ae82c0`。
- Executor: Codex Q1。前 Queue finished、並行 executor なしを確認。
- Applicable rules: AGENTS.md、Guardrail、coding-style.md 全文、WS105 D1〜D25、design.md の指定節。
- Prerequisites: Phase の依存と既存出力を実装・証拠で確認。material scope / criteria の変更・取消なし。
- Started UTC: 2026-10-01T10:50:48.162514+00:00

| Attempt | Phase | Status | Dependency | Selection reason |
| --- | --- | --- | --- | --- |
| q536-i01 | [ws105-p009](ws105/q536/phase.md) | cleared | p008、p005（logind fdの修復）（context） | WS105 の既存依存順 |

Dependency graph: Phase の依存（context）→ q536-i01/ws105-p009。他の Phase は次の Queue で選定する。

## Upcoming Work Outlook

WS105 の依存を満たす次の Phase。新しい product / risk 判断はユーザーの決定が前提。既存 WS105 の完了までの自走指示で選定・実行する。
GitHub への publication は deferred、outbox に保持する。

## Outcome

ws105-p009 **cleared**。

cleared（q536-i01）。p005のlogind fd修復5012d324を前提に、p009 source3400a098と入力lease補正8b0c6ee4で全7基準を検証。gcc14.2 / clang19.1.7 build warning0、26ELF（24本体+2fixture）、makefile-sync、331source header-check、変更Linux C / DBus fixture style-check0 PASS。公開keiland/OS API不変、compositor DRM ioctl0、toolchain変更なし。

Linux QEMU Debian13 gdm専用guest: 自動loginはuser kei、XDG_SESSION_TYPE=wayland / ID171 / RUNTIME/run/user/1000、escaped logind session path、wallpaper / systembarをPNGで確認。HomeからTerminal起動とecho入力PASS。SwitchTo・chvtの両方で同一PID7648を維持、DRMと4evdevの5leaseすべてのPauseDevice/ResumeDevice、復帰画面 / pointer / echo switch-ok・chvt-ok PASS。kernel revokeがD-Bus通知より先に届く入力ENODEVはleaseを保持しEAGAINとする補正、後のResumeDevice fdへ交換を実測。途中の補正前検証は原ログに保持。

V11の実観測: このsystemd257の両コマンドはtype=force（SwitchToをcooperative pauseと捏造しない）。DRM revocation後のCRTC restoreにPermissionDeniedが記録されるが、quiesce/output閉鎖→resume/swapchain再生成はPASS。pause-type ACK分岐はsource確認のみ、実guest通知未実施。外部deviceの実機hotplug / systemd再起動は未実施。

自動loginを切り、QMPでkei/passwordを入力→Keiland userkei PID8493→Home LogOut→gdm greeterへ戻るPNG PASS。最初のpassword入力はUI遷移待ち不足で拒否、focus後同じpasswordで成功（元PNGとlogを保持）。gdm guest停止、overlay破棄。baseguestも最新版stageをinstallし、root KEILAND_SEAT=direct / --session --glassを起動、wallpaper / systembar、Home Terminal echo keiland-direct-ok PASS。SIGTERM frames62/error0/cleanup_failed0、console復元、guest停止。

D-Bus実production clientの独立wire fixture: byte分割、call中2signal queue、各frameのSCM_RIGHTS分離/CLOEXEC/payload、返信serial、fd所有移管、missing right / 64KiB超過 / partialheader+right EOF / ancillary17fd truncationの拒否と全fd回収 PASS。同じ5caseのASan/UBSanも全PASS。

zedBSD: disk-image warning0、OS boundary / GPU V1、dedicated-host / gpu-zedbsd-host ordinary+sanitize、boot-test loginPNG PASS。C1/C2/C9は全13PASS。forge-guest PASS（3imports / 120frame）、fence-guest PASS（600fences / 600frame / generation1）。全target-regression PASS。

証拠 [q536 manifest](../../history/ws105/q536/evidence/SHA256SUMS)。PNG目視済み、代表画面は当チャットに表示。host追加package0、host画面/入力を使用せず、host /opt installなし。QEMUと実機を区別、実機未実施。BUG-125 / BUG-127は未修正tracking、前のq532 FAILを維持。GitHub publication / close はoutbox pending、pushなし。次は既存p010 network/ALSA、その後p011全文規約とWS最終受け入れ。


実装 commit `8b0c6ee4c6a26445503b67310d1bdc9f21fefe0b`。Finished UTC: 2026-10-01T11:23:22.269889+00:00。次の Phase は WS105 の既存完了までの自走指示で選定。push / GitHub 公開は未実施。
