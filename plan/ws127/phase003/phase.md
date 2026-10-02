<!-- awesome-plan project=zedbsd record=ws127-p003 -->

# ws127-p003: 名前の衝突と Trash の残り（F-050・F-041 の一部）

Status: planning（2026-10-02 ベータ1の計画。要る判断・成果は「未決の判断」と「依存」）
Disposition: normal
Parent: [WS127](../ws.md)
Queue: なし
依存: p001 でユーザーが採用
目安: 3h（1 Queue）。実行者の目安: phase-runner-mid
所有 path: `files/actions.c`・`task.c`・`trash.c`・`undo.c`・`clip.c`・`ui-overlay.c` の該当、`plan/ws127/tests/`

## 範囲

(1) Replace で置き換えた item を消さずに Trash へ（undo で戻せる）。(2) cut の paste を Esc で止めたとき clipboard を戻す。(3) folder の merge（中身を合わせ、子の衝突は同じ dialog）。(4) home の外の volume の `$topdir/.Trash-$uid`（freedesktop.org Trash）。

## 受け入れ

(1)〜(4) の host の model の試験と guest の手順が PASS、既存の衝突の dialog（ws035-p106）の試験 PASS。
変えた source の style-check の違反 0、build の warning 0、`files-regress.sh` の 14 本と boot test PASS。

## 未決の判断

採否（p001）。folder の merge を入れるか

## 検証の方法と範囲

QEMU の Venus（`plan/tools/files/files-guest.sh`、`files-regress.sh`）と host の試験。console・serial の log では判定しない。各 Phase の最後に `files-regress.sh` の 14 本と boot test。
Settings と共有の `canvas.c`・`text.c`・`icons.c` を変えたら `sh plan/ws089/tests/host-build.sh` と Settings の host 試験も流す。

## Event

2026-10-02 / ws127-beta1-plan: fg019 の計画で新設（planning）。p001 の結果とユーザーの選択で範囲を確定し planned にする。
