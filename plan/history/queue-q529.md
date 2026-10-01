<!-- awesome-plan project=zedbsd record=q529 -->

# Queue

Status: finished
Active Queue: なし
Last finished Queue: q529

## q529

- Purpose: compositor の Linux の build と module (1): seat-direct・入力・session（wl_shm の client まで）
- Focus: fg012 / WS105 の既存 Linux の受け入れを満たす。
- Timebox: 60 分。既知の build・必須検証は最大 150 分、未知の問題の調査は 30 分で結果と再開条件を記録する。
- Approval: current user, 2026-10-01「ws105の完了をゴールにして、自走をお願いします。」。既存 WS105 p001〜p011 の範囲を依存順の 1 Phase Queue で実装・検証・記録する。commit は WIP、push / GitHub 公開は行わない。
- Exact approved scope: [ws105-p006](ws105/q529/phase.md) 全範囲、開始前 [snapshot](ws105/q529/scope.md) SHA256 `18f3740d8f5cfa6afed2d8999200861f2a0686cc55c41b771cff1598a09766d0`。
- Executor: Codex Q1。前 Queue finished、並行 executor なしを確認。
- Applicable rules: AGENTS.md、Guardrail、coding-style.md 全文、WS105 D1〜D25、design.md の指定節。
- Prerequisites: Phase の依存と既存出力を実装・証拠で確認。material scope / criteria の変更・取消なし。
- Started UTC: 2026-10-01T08:13:22.040837+00:00

| Attempt | Phase | Status | Dependency | Selection reason |
| --- | --- | --- | --- | --- |
| q529-i01 | [ws105-p006](ws105/q529/phase.md) | uncleared | p005、**WS104 の完了**（compositor の `zwl-gpu.h`・`zwl-input.h`・`zwl-os.h`（7 つの hook）と `zedbsd/` の module）（context） | WS105 の既存依存順 |

Dependency graph: Phase の依存（context）→ q529-i01/ws105-p006。他の Phase は次の Queue で選定する。

## Upcoming Work Outlook

WS105 の依存を満たす次の Phase。新しい product / risk 判断はユーザーの決定が前提。既存 WS105 の完了までの自走指示で選定・実行する。
GitHub への publication は deferred、outbox に保持する。

## Outcome

ws105-p006 **uncleared**。

uncleared（q529-i01）。Linux compositor/direct seat/evdev/VT/handoff/GPUなしglobalと、独立Makefile・fonts・wlshmを実装した。gcc/clang exit0 warning0、12ELF / source-sync / 126header / scoped style PASS。

- guest: READY、Virtual-1 1280×800。wallpaper/bar (100,10)=#ebf9ec / (100,400)=#c5dde6。wl_shm center (640,400) background (224,235,248)→(0,0,255)、約6000frame表示。cursor差は(200,200)と(900,600)の付近。App HomeとEsc、Super+TabのWiseviewを確認。
- 手順のmeta単独でHomeという記述は実装と違った。既存c5-transitions.shと同じlauncher click (23,17) に訂正、keyboardはEsc/Super+Tabで確認。受け入れ範囲は同じ。
- WiseviewをEscで閉じる連続描画で `ZWL VULKAN_ERROR operation=submit result=-1000001004`、`ZWL FAILED site=compose_draw errno=5`、EXIT frames=6051 error5 cleanup_failed1。masterはseatのroot fdで保持。KMSが100ms総期限をOUT_OF_DATEとする実装を確認、設計は各pollの上限100msであり総期限の意図ではなかった。タイミングからpoll deadlineが原因と推定、次のp005修正Queueでbounded遅延試験により確かめる。
- Log Outは上記でcompositorが先に終了したため未達。この終了をLog Out成功とは数えない。独立したSIGTERM終了とseat未指定のSSH tty sessionではREADY→EXIT error0、chvt1文字とconsole key 'kei' PNGを確認。
- zedBSD disk-image exit0、C1〜C5 boundary、v1、dedicated/decode host、boot login PNG確認。C1/C2/C9＋forge/fenceの承認済み回帰processは進行中（exec session52085、outputs build/ws105-p006）。既存承認の検証だけを継続して証拠を保存し、再開Queueでterminal結果を確認する。これらを今PASSと扱わない。
- Linux guest stop済み、overlay廃棄。sourceはWIPに保存。mainの委任された技術判断でp005を再開し、poll1回≤100ms＋有限の総期限のKMS待機に直す。p005の修正と再検証後、同じp006を再開してLog Out・SIGTERM・keyboard・zedBSD回帰の全条件を確認する。WS105の受け入れは変更しない。


実装 commit `80eea509710d51928bf14efd1246c02c80da38f4`。Finished UTC: 2026-10-01T08:25:21.947014+00:00。次の Phase は WS105 の既存完了までの自走指示で選定。push / GitHub 公開は未実施。
