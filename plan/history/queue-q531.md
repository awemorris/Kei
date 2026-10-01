<!-- awesome-plan project=zedbsd record=q531 -->

# Queue

Status: finished
Active Queue: なし
Last finished Queue: q531

## q531

- Purpose: compositor の Linux の build と module (1): seat-direct・入力・session（wl_shm の client まで）
- Focus: fg012 / WS105 の既存 Linux の受け入れを満たす。
- Timebox: 60 分。既知の build・必須検証は最大 150 分、未知の問題の調査は 30 分で結果と再開条件を記録する。
- Approval: current user, 2026-10-01「ws105の完了をゴールにして、自走をお願いします。」。既存 WS105 p001〜p011 の範囲を依存順の 1 Phase Queue で実装・検証・記録する。commit は WIP、push / GitHub 公開は行わない。
- Exact approved scope: [ws105-p006](ws105/q531/phase.md) 全範囲、開始前 [snapshot](ws105/q531/scope.md) SHA256 `fb0b5a5fa90e5381315661fc19cf07414062c19ada0281bbce8ab5441843e9c7`。
- Executor: Codex Q1。前 Queue finished、並行 executor なしを確認。
- Applicable rules: AGENTS.md、Guardrail、coding-style.md 全文、WS105 D1〜D25、design.md の指定節。
- Prerequisites: Phase の依存と既存出力を実装・証拠で確認。material scope / criteria の変更・取消なし。
- Started UTC: 2026-10-01T08:31:05.517206+00:00

| Attempt | Phase | Status | Dependency | Selection reason |
| --- | --- | --- | --- | --- |
| q531-i01 | [ws105-p006](ws105/q531/phase.md) | cleared | p005、**WS104 の完了**（compositor の `zwl-gpu.h`・`zwl-input.h`・`zwl-os.h`（7 つの hook）と `zedbsd/` の module）（context） | WS105 の既存依存順 |

Dependency graph: Phase の依存（context）→ q531-i01/ws105-p006。他の Phase は次の Queue で選定する。

## Upcoming Work Outlook

WS105 の依存を満たす次の Phase。新しい product / risk 判断はユーザーの決定が前提。既存 WS105 の完了までの自走指示で選定・実行する。
GitHub への publication は deferred、outbox に保持する。

## Outcome

ws105-p006 **cleared**。

cleared（q531-i01）。Linux の seat-direct・入力・session・wl_shm の compositor を実装・検証した。p006 source は `80eea509`、依存 KMS repair は `753b45a0`（いずれも WIP）。q529 の uncleared は元の履歴として保持する。

- gcc14.2 / clang19.1.7 build exit0 warning0、12ELF / source-sync / 126header PASS。Linux の OS module は `linux/` に分離し、compositor は表示を Vulkan だけで扱う。evdev は monotonic clock、KD/keyboard mode は個別保存・復元。全文規約と style-check を点検した（最終 WS conformance は p011）。
- Linux guest: desktop / wlshm / Home / Esc / Wiseview と pointer 2 地点の PNG を確認。bar、wallpaper、青い window の画素、cursor の差分を確認。launcher は実際の top-left click を使い、Super+Tab は Wiseview（検証手順の補正、product code は不変）。
- Home の Log Out: `ZWL SESSION logout`、`ZWL EXIT frames=1081 error=0 cleanup_failed=0`。コンソールで `kei` が入力される PNG を確認。再起動は KEILAND_SEAT 未指定・XDG_RUNTIME_DIR=/run・--socket 未指定で `/run/wayland-keiland` に READY、SIGTERM は `frames=1 error=0 cleanup_failed=0`。PNG はユーザーに表示、guest は停止し overlay を破棄。
- zedBSD: REQUIRED_TARGET_RESULT
- [ログ・PNG・SHA256 manifest](../../history/ws105/q531/evidence/)。Target 回帰は q529 で開始した同一 source の直列実行を q531 へ引き継いだ。q530 は Linux KMS module のみの修正で、target source / toolchain はその間変更していない。
- 実機 GPU / 物理モニタ未実施。Linux GPU client は次 p007、logind は p009、app/data は p008、network/audio は p010 の既存範囲。GitHub 未公開、outbox に証拠・event と intended close を保持、push なし。


実装 commit `29cf10de919a5629b9e58387b8a5ff32c223cb76`。Finished UTC: 2026-10-01T08:50:34.308158+00:00。次の Phase は WS105 の既存完了までの自走指示で選定。push / GitHub 公開は未実施。
