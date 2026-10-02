<!-- awesome-plan project=zedbsd record=ws127-p006 -->

# ws127-p006: DnD の自動の scroll と spring-loaded（F-039）

Status: planning（2026-10-02 ベータ1の計画。要る判断・成果は「未決の判断」と「依存」）
Disposition: normal
Parent: [WS127](../ws.md)
Queue: なし
依存: p001 でユーザーが採用
目安: 2h（1 Queue）。実行者の目安: phase-runner-mid
所有 path: `files/ui-drag.c`・`dnd.c`・`ui-grid.c`・`ui-list.c`・`ui-tabs.c` の該当、`plan/ws127/tests/`

## 範囲

窓の中の DnD で、一覧の上下の端で自動の scroll、tab・folder・sidebar の上で 0.8 秒待つと開く（spring-loaded）。

## 受け入れ

guest の手順: 1000 項目の folder の下端へ drag して scroll が進む、folder の上で待って開いて落とせる。既存の DnD の試験（files-p010 ほか）PASS。
変えた source の style-check の違反 0、build の warning 0、`files-regress.sh` の 14 本と boot test PASS。

## 未決の判断

採否（p001）

## 検証の方法と範囲

QEMU の Venus（`plan/tools/files/files-guest.sh`、`files-regress.sh`）と host の試験。console・serial の log では判定しない。各 Phase の最後に `files-regress.sh` の 14 本と boot test。
Settings と共有の `canvas.c`・`text.c`・`icons.c` を変えたら `sh plan/ws089/tests/host-build.sh` と Settings の host 試験も流す。

## Event

2026-10-02 / ws127-beta1-plan: fg019 の計画で新設（planning）。p001 の結果とユーザーの選択で範囲を確定し planned にする。
