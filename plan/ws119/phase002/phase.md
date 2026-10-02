<!-- awesome-plan project=zedbsd record=ws119-p002 -->
# ws119-p002: 導入の backend

Status: planning（p001 とユーザーの要件の判断を待つ）
Disposition: normal
Parent: [WS119](../ws.md)
Focused goal: fg019（ベータ1）
Queue: none
目安: 3〜4h

## 範囲

p001 の design.md の interface に沿って、非対話の C の command（名前は p001 で決める、例 `/sbin/kei-install`）を `userland/base/` か `userland/desktop/` の新しい directory に作る:
導入の計画（disk、利用者など）を受けて、GPT の作成（`diskpart`）、ESP の FAT32 と UFS の root と swap の format（`mkfs`・`mkswap`）、live の root と ESP の複写、`zedbsd.cfg`・`/etc/fstab` の書き換え、
利用者の作成、sync と検証。進捗と失敗の段階を機械が読める行で出す（frontend が読む）。失敗の途中で導入先が起動可能に見えないこと（最後に boot の設定を公開する）。

## 受け入れ

QEMU の NVMe の空の disk に CLI で導入し、USB を外して起動して login prompt（`plan/tools/boot-test.sh` の形、PNG をユーザーに見せる）。容量不足・取消しの試験。build warning 0、全文規約。

## 所有 path

新しい backend の directory、`plan/ws119/`。`diskpart`・`mkfs` 等の base の道具に足りない機能があれば p001 で範囲を決め、その file だけを変える。

## 依存

p001、ユーザーの判断。

## 未決の判断

p001 の結果による。
