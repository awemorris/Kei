<!-- awesome-plan project=zedbsd record=ws089-p015 -->

# ws089-p015: 日本語の UI

Status: planning（ユーザーの判断: 日本語の UI をベータ1 に入れるか）
Disposition: normal
Parent: [WS089](../ws.md)
Queue: なし
依存: p010、WS127 p005 と共通の仕組み（先に作った側に合わせる）
目安: 3h（1 Queue）。実行者の目安: phase-runner-mid
所有 path: `userland/desktop/settings/` の文言、共有の仕組みの置き場所は Q1

## 範囲

頁・項目・検索の語を message の catalog に出し、`LANG=ja_JP.UTF-8` で日本語に。検索は英語と日本語の両方の語で引ける。

## 受け入れ

日本語の画面（全頁）、英語の既定が変わらない（既存の回帰 PASS）、日本語の語で検索できる（guest）。

## 検証の方法と範囲

QEMU の Venus。 やっていない確認は「未実施」と書く。

## 未決の判断

ベータ1 に入れるか（ユーザー）。仕組みの置き場所（Q1）。

## Event

2026-10-02 / ws089-beta1-plan: fg019 の計画で新設。
