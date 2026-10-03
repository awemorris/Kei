<!-- awesome-plan project=zedbsd record=ws131-p018 -->

# ws131-p018: Terminal・Notes を新しい API へ（DnD・primary・tablet・fd の監視）

Status: planning（p002 第 2 版はユーザーのレビュー済み（2026-10-03、D7 は確認中）。開始はユーザーの承認と P2 の終了の後に Q1 が指示）
Disposition: normal
Parent: [WS131](../ws.md)、計画の正本 [design.md](../design.md)
Queue: none
依存: p016 cleared。D8 の単独走行で番号の順
目安: 4〜5h（1 Queue）。実行者: Q1 が割り当てる（high）
所有 path: `userland/desktop/terminal/`・`notes/`、`libkeiland/app/`・`ui/`（DnD の受け・primary・tablet・fd の監視）、`plan/ws131/`

## 目的と結果

Terminal（pty の fd・自前の clipboard 808 行と primary 330 行）と Notes（tablet の自前の registry）の自前の registry を除き、libkeiland に DnD の受け・primary・tablet・`kl_app_watch_fd` を足して移す。Terminal の pty は Terminal に OS の依存を残す（D14、2026-10-03 user）: `TIOCSWINSZ` は 3 OS で同じで、`openpty` の header（`<pty.h>`／FreeBSD の `<libutil.h>`）だけを macro の block で切り替え、checker の許可の表に載せる。

## 範囲

1. libkeiland: `kl_window_accept_drops`、primary（自分の選択を自分で paste する時の詰まりの回避を保つ）、tablet の event、窓ごとの repeat の無効、全 motion。
2. Terminal・Notes の menu（413・316）・tabs を宣言的に。scroll の model の移行は WS090 の残りとして WS131 の完了の後に扱う（D9 の決定）。

## 受け入れ

- 3 OS の build（zedBSD の amd64 は exit 0・自前の warning 0、`make keiland-linux` の gcc と clang、FreeBSD の native build と `native-build-audit.py`）と `plan/tools/keiland-os-boundary/check.sh` PASS。
- 対象の app に旧名（`kui_`・`KUI_`・`keiland_`・`KEILAND_`）と自前の menu・titlebar・glass の結線・main loop が無い（grep）。結線の行数の減りを記録。
- zedBSD: `plan/ws081/tests/run-termtouch.sh`・`run-notestouch.sh`、`plan/ws128/tests/notes-p002.sh`、`demo-s8-s9.sh`、Terminal の clipboard・PRIMARY・drop の guest の手順、boot-test。Linux: 2 app の PNG と Terminal の shell。

## 検証の方法

[zedbsd-commands.md](../../tools/keiland-linux/zedbsd-commands.md)、[keiland-linux/README.md](../../tools/keiland-linux/README.md)、[keiland-freebsd/README.md](../../tools/keiland-freebsd/README.md)。QEMU の console・serial の log で判定しない（不具合は gdbstub・monitor・QMP）。build は自分の `BUILD=build/ws131-pNNN/`、共有の `build/` を消さない。FreeBSD の起動の確認は判断 D11 の範囲（passthrough なし）。QEMU と実機の証拠を分ける。

## 衝突・危険・rollback

- 衝突: WS128・WS079 と同じ file。 順は Q1 が決める（design.md §7.2）。
- rollback: 前へ直すのを基本にし、後ろに同じ file の変更が無い時だけ統合の commit を revert する（design.md §7.4）。

## Resume

依存の Phase の cleared と main への統合、関係する判断の決定の後に、Q1 が Queue を作る。
