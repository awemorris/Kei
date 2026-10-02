<!-- awesome-plan project=zedbsd record=ws089-p013 -->

# ws089-p013: About の memory と Storage の使用量

Status: planning（libkeiland の API の追加の main の許可、p010 のユーザーの採否が要る）
Disposition: normal
Parent: [WS089](../ws.md)
Queue: なし
依存: p010、[proposed/libkeiland-system.md](../proposed/libkeiland-system.md) の許可（KEILAND_VERSION を上げる → WS113 p005 と直列）
目安: 2h（1 Queue）。実行者の目安: phase-runner-mid
所有 path: `userland/desktop/libkeiland/`（system の照会）、`include/libc/keiland.h`、`userland/desktop/settings/about.c`・`page-about.c`

## 範囲

About の頁に memory（全体と使用中）を出す。Storage の頁に volume ごとの使用量（statvfs）が無ければ足す。

## 受け入れ

About に memory の行、host の試験（`host-render.c` の about）と guest の画面。libkeiland の host 試験 PASS。

## 検証の方法と範囲

host と QEMU の Venus。 やっていない確認は「未実施」と書く。

## 未決の判断

libkeiland の API の追加の許可（main）と採否（ユーザー）。

## Event

2026-10-02 / ws089-beta1-plan: fg019 の計画で新設。
