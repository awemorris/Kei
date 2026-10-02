<!-- awesome-plan project=zedbsd record=ws099-p023 -->
# ws099-p023: BUG-136（Gears のタイトルバー）と BUG-137（Terminal のタイトルバーの遅れ）

Status: planned
Disposition: normal
Parent: [WS099](../ws.md)
Bugs: [BUG-136](../../bugs/BUG-136.md)、[BUG-137](../../bugs/BUG-137.md)

## 範囲（2026-10-02 ユーザーの実機の指摘）

1. BUG-136: X11 の Gears（`keiland-x11 zgears`、rootless）の窓に Keiland のタイトルバーが出ない原因を調べて直す。他の X11 の窓（zterm）も確かめる。
2. BUG-137: Terminal の初回の起動で、窓の本体の数秒後にタイトルバーが出る。初回だけの読み込み（font・icon・cache、BUG-135 の stat）と、titlebar の surface・menu の初期化の順を、時刻の計測で特定して直す。
3. QEMU の Venus で再現と修正の確認（起動直後の PNG を時刻つきで連写、Terminal の初回・2 回目・3 回目）。build warning 0、C 全文規約、変えた領域の試験、最後に boot-test。実機の確認はユーザー（未実施と書く）。

## 所有 path

`userland/desktop/wayland/` の titlebar・x11 の関係の file、`userland/desktop/terminal/`、`userland/desktop/x11server/`（要る範囲）、`plan/ws099/`。
