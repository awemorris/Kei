<!-- awesome-plan project=zedbsd record=q527 -->

# Queue

Status: finished
Active Queue: なし
Last finished Queue: q527

## q527

- Purpose: libvulkan-compat (2): Wayland の WSI（`zwp_linux_dmabuf_v1`、implicit sync）
- Focus: fg012 / WS105 の既存 Linux の受け入れを満たす。
- Timebox: 60 分。既知の build・必須検証は最大 150 分、未知の問題の調査は 30 分で結果と再開条件を記録する。
- Approval: current user, 2026-10-01「ws105の完了をゴールにして、自走をお願いします。」。既存 WS105 p001〜p011 の範囲を依存順の 1 Phase Queue で実装・検証・記録する。commit は WIP、push / GitHub 公開は行わない。
- Exact approved scope: [ws105-p004](ws105/q527/phase.md) 全範囲、開始前 [snapshot](ws105/q527/scope.md) SHA256 `956c51e4057a1acc221b3d3a291739b7ce5ec54efadc5f41970b32c085112509`。
- Executor: Codex Q1。前 Queue finished、並行 executor なしを確認。
- Applicable rules: AGENTS.md、Guardrail、coding-style.md 全文、WS105 D1〜D25、design.md の指定節。
- Prerequisites: Phase の依存と既存出力を実装・証拠で確認。material scope / criteria の変更・取消なし。
- Started UTC: 2026-10-01T07:23:14.983218+00:00

| Attempt | Phase | Status | Dependency | Selection reason |
| --- | --- | --- | --- | --- |
| q527-i01 | [ws105-p004](ws105/q527/phase.md) | cleared | p003（context） | WS105 の既存依存順 |

Dependency graph: Phase の依存（context）→ q527-i01/ws105-p004。他の Phase は次の Queue で選定する。

## Upcoming Work Outlook

WS105 の依存を満たす次の Phase。新しい product / risk 判断はユーザーの決定が前提。既存 WS105 の完了までの自走指示で選定・実行する。
GitHub への publication は deferred、outbox に保持する。

## Outcome

ws105-p004 **cleared**。

- **cleared**。q526 の旧条件が不成立だった履歴を保持し、kernel の stub に依存しない改訂検証を実施した。product API / D6 の implicit sync と CPU fallback は同じ。
- gcc 14.2.0 / clang 19.1.7 の最終 source build warning 0。`elf-check: PASS`（8 ELF）、`makefile-sync: PASS`、`header-check: PASS`（64 source）。`git diff --check` PASS。libvulkan-compat 全 C/header、変更した試験 C と generated forward.inc の style-check 合計0、ANSI 宣言・public/static順・callback storage寿命・fd ownership・error unwindを全文規約で照合。
- `timeout 120 bash plan/tools/keiland-linux/wsi-check.sh`: FIFO/fallback/resize/MAILBOX は各90 frame、client/server exit0。全360 frame の実画素がN%3の赤/緑/青と一致。resizeは320×240→400×300→320×240、MAILBOXも90frame。
- 通常3 run: IMPORT_SYNC_FILE flags=WRITE(2) がそれぞれ90回成功。private waitは各180回（acquire/reuse）、全て成功。fallback: ENOTTY(25)は最初の1回のみ、以後import試行無し。private waitは270回、CPU-before-commitが全90frameに加わり全て成功。observerは試験専用、backend DEEPBINDとproduction引数/戻り値を維持。
- raw SYNC_IOC_FILE_INFO は全runでfences=1、driver/timeline=stub、status=1、waited_ms=0。値を加工して0にせず保存。この値だけをimplicit sync成功の根拠にしない。V3: lavapipeの実際のcreated modifier=0x0、single plane、stride=1280/1600、offset=0を確認。V9はkernel importの成功と実画素で検証。
- `vk-chain-test: PASS`: staged SONAME、surface/wayland拡張有り、XCB/Xlib無し、未enableのWayland procedure=NULL、API1.0、llvmpipe、1MiB fill/copy一致。`interpose-check: PASS`、default backend-to-compat bindings=0、NO_DEEPBIND optoutもPASS。
- `make -j4 disk-image` exit0、warning0。Linux固有library/testだけの変更のためzedBSD runtime回帰はp011の全体回帰で実施。host package追加・target toolchain変更・host /opt install無し。
- swapchain destroy時にGPU資源を先に退役し、未releaseのWayland callback storageだけをsurfaceで保持する。deviceが先に破棄されてもcallback dataが残る。初回acquireより前のapplication queue retrievalを必須にしない。
- [raw evidence](../../history/ws105/q527/evidence/) と [ELF manifest](../../history/ws105/q527/manifest.sha256)を保存。valgrindはhostに無く未実施。実機GPUの非同期待ちは未実施、host lavapipeのみ。GitHub publication/remote closeはdeferred、outboxで保持。commit WIP、push無し。


実装 commit `cb6a9eacf1dac1a1f6381809ba102558ffc41467`。Finished UTC: 2026-10-01T07:35:44.278649+00:00。次の Phase は WS105 の既存完了までの自走指示で選定。push / GitHub 公開は未実施。
