<!-- awesome-plan project=zedbsd record=ws128-p005 -->

# ws128-p005: Image Viewer の改善

Status: planning（p001 でユーザーの採否）
Disposition: normal
Parent: [WS128](../ws.md)
Queue: なし
依存: p001
目安: 2h（1 Queue）。実行者の目安: phase-runner-mid
所有 path: `userland/desktop/imageview/`（`image.c` は WS094 p014 と直列、`picture/` を変えるなら WS127 p004 と直列）

## 範囲

候補: File > Move to Trash（Files の Trash の規則、次の画像へ進む）、Open With（WS093 の対応）、slideshow（全画面で一定の間隔）、Edit > Copy（画像を clipboard へ、image/png）。p001 で選んだ物だけ。

## 受け入れ

選んだ項目ごとの guest の手順と host の試験 PASS、既存の `run-host.sh`・`imageview-guest.sh` PASS、boot test PASS。

## 検証の方法と範囲

host と QEMU の Venus。 やっていない確認は「未実施」と書く。

## 未決の判断

採否（ユーザー）。

## Event

2026-10-02 / ws128-beta1-plan: fg019 の計画で新設。
