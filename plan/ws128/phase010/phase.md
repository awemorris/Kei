<!-- awesome-plan project=zedbsd record=ws128-p010 -->

# ws128-p010: Terminal の右端の wrap（BUG-150、Emacs の画面が 1 行ずれる）

Status: cleared（Q1 照合済み 2026-10-03、統合 02c0d6f41、zedBSD image の build warning 0 も P2 が確認。元の記載: cleared（q636-i01、P2 generation4、2026-10-03。user の指示で絞った検証（FreeBSD 実機の native build と host 試験、zedBSD の build warning 0）で PASS。Q1 の照合待ち）
Disposition: normal
Parent: [WS128](../ws.md)
Queue: q636 / q636-i01（承認: user 2026-10-03「BUG-150は今実行してOKです。FreeBSDホストを空けたので使ってください。Emacsは入ってます。」。時限 3h）
Bug: [BUG-150](../../bugs/BUG-150.md)
依存: なし。WS131 p018・ws128-p006 と同時に流さない（同じ `userland/desktop/terminal/`）
目安: 1〜3h（1 Queue）。実行者: phase-runner（P2）
所有 path: `userland/desktop/terminal/`、`plan/ws128/`（phase010・tests・ws.md）、`plan/bugs/BUG-150.md`

## 範囲

1. FreeBSD 実機（Latitude 5320、FreeBSD 15.1-RELEASE）で `TERM=xterm emacs -nw -Q` が pty に書く列を記録し、FreeBSD と Linux の xterm の terminfo を比べる。
2. 記録した列を Terminal の `screen.c` に流す host 試験を作り、原因を file・行で特定して直す。
3. zedBSD・Linux・FreeBSD の build。
範囲の外: alternate screen（`CSI ?1049h`）の正しい保存と復元、DECAWM（`CSI ?7l`）、DA・DSR などの応答（今は返さない）。

## 受け入れ

| # | 条件 | 確かめ方 |
| --- | --- | --- |
| A1 | 記録した Emacs の最初の画面（FreeBSD の Emacs 31.1、Debian の Emacs 30.1、80x24）を Terminal の grid に流すと、menu bar が 1 行目、buffer が 2 行目から、mode line が 23 行目、cursor が point の行 | host `plan/ws128/tests/terminal-p010.sh`（直す前 FAIL、後 PASS） |
| A2 | 右端の扱いが xterm（terminfo の am・xenl）と同じ: 最終列に字を置くと cursor は最終列に留まり wrap は保留、次の印字で wrap、CR・LF・BS・TAB・移動・編集の CSI で取消、SGR・mode は保持。最下行を埋めても scroll しない | 同じ試験の `wrap` |
| A3 | zedBSD の Terminal の build の warning 0、FreeBSD 実機の native build | build |

検証は 2026-10-03 user の指示で絞る（Q1 経由）:「FreeBSDで問題なく動作したら、test passとしてマージしていいです。詳細なテストやりなおしは不要です。軽微で自明な修正だからです。」→ FreeBSD 実機の native build と terminal-p010.sh、zedBSD の Terminal の build warning 0。Linux の build・既存の Terminal の試験・boot test は不要（S1 の boot test で代える）。

## 原因（特定済み）

- `userland/desktop/terminal/screen.c` の `screen_put()`（修正前の 1335〜1340 行）が、字を最終列に置いた直後に cursor を次の行の先頭へ移していた（即時の wrap）。xterm と VT100 は最終列に留まって wrap を保留する（terminfo の `xenl`）。
- Emacs は menu bar と mode line を 80 桁ちょうど書いてから `\r\n` で次の行へ進み、残りは `\e[A`・`\n`・`\e[K` の相対の移動で描く。即時の wrap では 1 行余分に下がり、mode line（23 行目）を書くと最下行で scroll して画面全体が上へずれ、menu bar が scrollback へ消える（修正前の replay: 1 行目が空、mode line が 22 行目）。
- CUP（`H`・`f`）・VPA（`d`）・CHA（`G`）・DECSTBM（`r`）の 1 起点の扱いは正しかった。FreeBSD の xterm の entry（`/etc/termcap.db` から、`smcup`・`rmcup` が無い）と Linux（ncurses、`smcup=\E[?1049h\E[22;0;0t`）の違いは原因ではない（Emacs の描画の列は両方で同じ形）。zedBSD・Linux でも同じ source なので同じく起きる（host の replay で Linux の記録も修正前 FAIL）。

## 記録した列

- `plan/ws128/tests/terminal-p010-emacs-freebsd.bin`（524 byte、sha256 `4252aed6…cd79`）: FreeBSD 15.1、Emacs 31.1、`TERM=xterm`、80x24、`emacs -nw -Q` の起動から 4 秒（key を送る前）。
- `plan/ws128/tests/terminal-p010-emacs-linux.bin`（1310 byte、sha256 `12b407ee…da51`）: Debian 13、Emacs 30.1、同じ条件。
- 記録の道具 `plan/ws128/tests/terminal-p010-capture.py`（pty を開き、子の起動の前に `TIOCSWINSZ` で大きさを決め、出力を記録し、`C-x C-c` を送る）。

## 修正

- `terminal.h`: `struct terminal_screen` に `wrap_pending`。
- `screen.c`: `screen_put()` は最終列に置いた字の後 cursor を最終列に留め `wrap_pending = 1`。次の印字の前に wrap する。CR・BS・TAB（`screen_byte`）、`screen_line_feed`・`screen_reverse_index`・`screen_move`・`screen_clear_grid`・resize、cursor を動かすか grid を変える CSI（`SCREEN_WRAP_CANCELS` の final）で取消。SGR・`h`/`l`・private の列は保持。

## 実行の記録（q636-i01、P2 generation4、2026-10-03）

| 確認 | 環境 | 結果 |
| --- | --- | --- |
| 記録（`terminal-p010-capture.py`、`TERM=xterm emacs -nw -Q`、80x24） | FreeBSD 実機 5320（`~/zedbsd-p2/`）と Linux host | 上の 2 file。`infocmp xterm` は FreeBSD が `/etc/termcap.db` 由来（`smcup` 無し）、Linux は ncurses（`smcup` あり）。どちらも `am`・`xenl` |
| host `plan/ws128/tests/terminal-p010.sh`（修正前の source、git stash） | Linux host（gcc） | **FAIL**（wrap 8 項目 FAIL、FreeBSD の replay は 1 行目が空・mode line が 22 行目、Linux の replay も同じずれ） |
| host `plan/ws128/tests/terminal-p010.sh`（修正後） | Linux host（gcc 14、`-Werror`） | **PASS**（wrap 12 項目、emacs FreeBSD 5 項目、emacs Linux 4 項目） |
| host `sh plan/ws128/tests/terminal-p010.sh build/ws128-p010`（修正後） | **FreeBSD 実機** 15.1-RELEASE、clang 19.1.7、`-Werror` | **PASS**（同じ 21 項目） |
| native build `gmake -j8 -f userland/desktop/keiland-freebsd.mk KEILAND_FREEBSD_BUILD=build/native-p2 all`（`~/zedbsd-p2/src`、worktree の tracked file の複写＋修正） | FreeBSD 実機 | exit 0、warning 0、`build/native-p2/bin/terminal` |
| zedBSD amd64（`plan/ws089/tests/build-settings-image.sh build/p2-p009-img`、clang `-Wall -Wextra -Werror`） | Linux host の cross build | `terminal/screen.c` を含む Terminal の compile・link は warning 0（log の `warning:` 0 件） |
| Linux native `make keiland-linux KEILAND_LINUX_BUILD=build/p2-keiland-linux` | Linux host | rc 0、warning 0（user の指示で不要になったが、指示の前に走り終えた） |

未実施（user の指示で不要、2026-10-03）: 既存の Terminal の試験（p009・menu-p003）の流し直し、boot test（S1 の boot test で代える）、Terminal の実物での目視（FreeBSD・zedBSD・Linux）。QEMU の確認は無い。

FreeBSD の画面で目視する時: 5320 で Keiland の session の中の shell から `~/zedbsd-p2/src/build/native-p2/bin/terminal` を起動し（install 済みの `/opt/keiland` は変えていない）、その中で `TERM=xterm emacs -nw -Q`。menu bar が 1 行目、mode line が下から 2 行目にあれば直っている。
