<!-- awesome-plan project=zedbsd record=ws128-p006 -->

# ws128-p006: Terminal の改善

Status: planning（p001 でユーザーの採否。WS090 p015 と file が重なる）
Disposition: normal
Parent: [WS128](../ws.md)
Queue: なし
依存: p001。WS090 p015（Terminal の scroll の `kui_scroll` への移行）と同時に流さない
目安: 2h（1 Queue）。実行者の目安: phase-runner-mid
所有 path: `userland/desktop/terminal/`

## 範囲

候補: scrollback の検索（Ctrl+Shift+F、一致の強調）、色の theme と font の大きさの保存（`~/.config/keiland/terminal`）、URL の click で開く。p001 で選んだ物だけ。

## 受け入れ

選んだ項目ごとの guest の手順 PASS、既存の Terminal の試験（p001 で特定）PASS、boot test PASS。

## 検証の方法と範囲

QEMU の Venus。 やっていない確認は「未実施」と書く。

## 未決の判断

採否（ユーザー）。WS090 p015 との順（Q1）。

## Event

2026-10-02 / ws128-beta1-plan: fg019 の計画で新設。
