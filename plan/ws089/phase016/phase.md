<!-- awesome-plan project=zedbsd record=ws089-p016 -->

# ws089-p016: 単一の instance

Status: planning（compositor の activation の仕組みが要る）
Disposition: normal
Parent: [WS089](../ws.md)
Queue: なし
依存: p010、compositor の Phase（WS099 と直列）
目安: 2h（1 Queue）。実行者の目安: phase-runner
所有 path: `userland/desktop/settings/main.c`、compositor・libkeiland の activation（所有は Q1 が決める）

## 範囲

二つ目の `settings` の起動（App Home・Files の desktop の menu の Change Wallpaper・`--page=`）で、既存の窓を前に出し指定の頁を開く。今の zdesktop に xdg-activation は無いので、仕組み（xdg-activation-v1 か libkeiland の小さな拡張か、Settings の UNIX socket か）を先に決める。

## 受け入れ

二つ目の起動で窓が 1 つのまま前に出て頁が替わる（guest の log と画面）。

## 検証の方法と範囲

QEMU の Venus。 やっていない確認は「未実施」と書く。

## 未決の判断

仕組みの選択（compositor に足すなら main の許可）。

## Event

2026-10-02 / ws089-beta1-plan: fg019 の計画で新設。
