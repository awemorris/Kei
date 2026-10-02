<!-- awesome-plan project=zedbsd record=ws089-p012 -->

# ws089-p012: Settings の中だけで済む操作性（検索の key・touch の scroll）

Status: planned（2026-10-02。Queue なし）
Disposition: normal
Parent: [WS089](../ws.md)
Queue: なし
依存: なし（p010 と並列可。p010 は source を変えない）
目安: 2h（1 Queue）。実行者の目安: phase-runner-mid
所有 path: `userland/desktop/settings/search.c`・`ui.c`・`pages.c`・`page-*.c` の入力の所、`plan/ws089/tests/`

## 範囲

1. 検索の結果の頁: 上下の key で選び、Enter で開く。titlebar の欄から Down で結果へ。選んだ行の見た目（Files の選択と同じ青）。
2. 頁の pane の touch: 1 本指の drag で scroll（慣性は libkeiland の `keiland_scroller` か Settings の今の scroll の model に合わせる。libkeiui への移行はしない）。
3. 左の項目の pane の Up・Down・Home・End の key の移動を見直し、頁の切り替えと履歴（戻る・進む）が崩れないこと。
範囲の外: libkeiui への移行（WS090 p007）、新しい頁、compositor。

## 受け入れ

- guest の新しい手順（`settings-p012.sh`）: Ctrl+F → `wall` → Down → Enter で Wallpaper の頁が開く（log `SEARCH open page=wallpaper`）、touch の drag で頁の pane が scroll（log の scroll の位置と画面）。
- `settings-regress.sh`（8 本）・host の試験 PASS、style-check の違反 0、build の warning 0、boot test PASS。

## 検証の方法と範囲

QEMU の Venus（touch は pen の image の注入）。console・serial の log では判定しない。 やっていない確認は「未実施」と書く。

## 未決の判断

なし（p010 の候補で別の優先が選ばれたら Q1 が順を変える）。

## Event

2026-10-02 / ws089-beta1-plan: fg019 の計画で新設。
