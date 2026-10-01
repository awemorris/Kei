<!-- awesome-plan project=zedbsd record=q526 -->

# Queue

Status: finished
Active Queue: なし
Last finished Queue: q526

## q526

- Purpose: libvulkan-compat (2): Wayland の WSI（`zwp_linux_dmabuf_v1`、implicit sync）
- Focus: fg012 / WS105 の既存 Linux の受け入れを満たす。
- Timebox: 60 分。既知の build・必須検証は最大 150 分、未知の問題の調査は 30 分で結果と再開条件を記録する。
- Approval: current user, 2026-10-01「ws105の完了をゴールにして、自走をお願いします。」。既存 WS105 p001〜p011 の範囲を依存順の 1 Phase Queue で実装・検証・記録する。commit は WIP、push / GitHub 公開は行わない。
- Exact approved scope: [ws105-p004](ws105/q526/phase.md) 全範囲、開始前 [snapshot](ws105/q526/scope.md) SHA256 `e09e0b484a935c26def605973cdbd2039dae8507f1ac4da9c14dde4fad6217a0`。
- Executor: Codex Q1。前 Queue finished、並行 executor なしを確認。
- Applicable rules: AGENTS.md、Guardrail、coding-style.md 全文、WS105 D1〜D25、design.md の指定節。
- Prerequisites: Phase の依存と既存出力を実装・証拠で確認。material scope / criteria の変更・取消なし。
- Started UTC: 2026-10-01T06:37:09.819193+00:00

| Attempt | Phase | Status | Dependency | Selection reason |
| --- | --- | --- | --- | --- |
| q526-i01 | [ws105-p004](ws105/q526/phase.md) | uncleared | p003（context） | WS105 の既存依存順 |

Dependency graph: Phase の依存（context）→ q526-i01/ws105-p004。他の Phase は次の Queue で選定する。

## Upcoming Work Outlook

WS105 の依存を満たす次の Phase。新しい product / risk 判断はユーザーの決定が前提。既存 WS105 の完了までの自走指示で選定・実行する。
GitHub への publication は deferred、outbox に保持する。

## Outcome

ws105-p004 **uncleared**。

- **uncleared**: 4 種の client/server がともに exit 0、各 90 frame。FIFO・resize・MAILBOX の全画素/サイズ一致。fallback の画素/サイズは一致したが、承認済み `fences=0` は不成立。
- host kernel `6.12.101+deb13-amd64`、lavapipe。全 run の raw fences=1、driver/timeline=`stub`、status=1、waited_ms=0。既に完了した writer と空の reservation をこの観測だけでは区別できない。fences≥1 だけによる通常経路の受け入れも根拠が不足する。
- Linux 6.12 の一次資料: [dma-buf.c](https://raw.githubusercontent.com/torvalds/linux/v6.12/drivers/dma-buf/dma-buf.c) の dma_buf_export_sync_file は空の reservation に dma_fence_get_stub を補う。[dma-fence.c](https://raw.githubusercontent.com/torvalds/linux/v6.12/drivers/dma-buf/dma-fence.c) の stub は既に signal 済み。
- gcc の Linux WSI build warning 0。4 run の [観測](evidence/) を保存。clang/全規約/chain 回帰はこの試行では未実施。既存コードの partial resource cleanup と通常 acquire の初期 queue 記録を次試行で見直す。
- 再開条件: 試験だけの ioctl observer で通常 IMPORT_SYNC_FILE の成功/flags=WRITE、fallback ENOTTY 1 回・以後成功 import 0 回、CPU fence 待ち・90 frame 全画素を実測する。raw fence/timeline は加工せず残す。D6 の production 同期方式、L3 の画素/安全性、他 Phase の出力は変更しない。main の委任済み技術判断で検証の観測方法を改訂し、新 Queue snapshot に残す。
- valgrind は host に無く未実施。実機 GPU の非同期 fence 待ちは未実施、lavapipe のみ。push/GitHub 公開なし。


実装 commit `e4588580404893bdc6f0affd33d68c064ecdeaa3`。Finished UTC: 2026-10-01T07:22:11.469836+00:00。次の Phase は WS105 の既存完了までの自走指示で選定。push / GitHub 公開は未実施。
