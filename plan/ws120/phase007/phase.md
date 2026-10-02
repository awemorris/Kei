<!-- awesome-plan project=zedbsd record=ws120-p007 -->

# ws120-p007: 全文規約と回帰

Parent: [WS120](../ws.md)
Status: planning
Disposition: normal
Queue / attempts: none
Goal: WS の全変更（libkeiland の再生の API、decoder の library、Music、Files の関連付け）を全文規約で見直し、3 OS の build と回帰、制限を整理する。
Prerequisites: p005 cleared（p006 を行うならそれも）。
Investigation bound: 2 時間。

## 範囲

- [coding-style](../../coding-style.md) の全文の review、build の warning 0（zedBSD・Linux・FreeBSD）。
- M1〜M6 の再実行、decoder の host 試験、libkeiland・Files の既存の試験、boot test。制限（対応しない形式、他 OS の再生、実機の未実施）を ws.md に。

## 受け入れ

違反 0 または承認済みの例外。M1〜M6 PASS。

## 検証

style の確認、host 試験、QEMU の試験、`plan/tools/boot-test.sh`。

## 所有 path

WS120 の全所有 path、`plan/ws120/`。

## 依存・未決の判断

p005（p006）。
