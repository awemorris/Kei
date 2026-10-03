<!-- awesome-plan project=zedbsd record=ws099-p029 -->
# ws099-p029: BUG-142・BUG-141 — compositor の入力の順と、zdesktop が pointer を取った時の leave

Status: cleared（q645、P2、2026-10-03。T2-008 で再試験 PASS、Q1 判定）
Disposition: normal
Parent: [WS099](../ws.md)
Bugs: [BUG-142](../../bugs/BUG-142.md)、[BUG-141](../../bugs/BUG-141.md)

## 範囲（Q1、2026-10-03 user「対応可能なバグ修正はないんですか」）

どちらも unreproduced の Files の bug（ws127-p001 の観察）。ticket と ws127 の記録を読み、原因の候補があれば直す。無ければ再現の試みを T1 に。
読みの結果、どちらも compositor（`userland/desktop/wayland/`）の側に候補があったので、WS099 で直す。

## BUG-142: 起動直後の Ctrl+C が次の click の後に処理される

- 読み: Files の側は順を保つ（menu の action も pointer・key も同じ queue、`fm_window_action`）。compositor の event loop（`main.c`）は
  ready な入力 device を **1 つずつ順に** 全部読んでいた（`zwl_input_read` を device ごとに、`INPUT_READS_PER_PASS` まで）。zdesktop が
  忙しい間（起動直後の import、guest の停止など）に keyboard の Ctrl+C と tablet の click が両方たまると、次の pass では device の並び
  （slot の順）で適用され、pointer の device が先なら click が Ctrl+C より先に Files に届く。観察（起動直後の 2.6 秒の停止の間の QMP の入力）と合う。
- 直し: `input.c` に `zwl_input_read_devices`（ready な device をまとめて読み、evdev の event の時刻の順に k 本の merge で適用。同じ時刻は
  device の順、各 device の中の順は保つ、各 device の読みの上限 `INPUT_READS_PER_PASS` も保つ）。report は device ごとに SYN_REPORT まで
  device の中に貯めてから適用するので、event の単位の merge で device の間の順は SYN_REPORT の時刻の順になる。`main.c` は ready な device を
  集めて 1 回呼ぶ。`zwl_input_read` は使う所が無くなったので外した。kernel の input の時刻は ms の分解能（`report_timestamp`）。
- 試験: `plan/ws099/tests/bug142-order.sh`（Report.pdf を選び、zdesktop を SIGSTOP（忙しい compositor の代わり）、Ctrl+C → Downloads の
  click → SIGCONT。Files の log で CLIPBOARD が LOCATION Downloads より先、Ctrl+V で Report.pdf が Downloads に）。直す前の compositor で
  再現するかは device の slot の順による（keyboard が先なら直す前でも通る）。

## BUG-141: Files の hover の強調が pointer が窓を出ても残る

- 読み: `seat.c` の `zwl_seat_motion` は、zdesktop の画面・menu・gesture が motion を取ると（`zwl_seat_motion_shell` が 1）、client への
  配達（`zwl_seat_pointer_update` → enter/leave）をしない。pointer が Files の item の上から zdesktop の menu（system bar の menu・F10・
  context menu）や App Home・Wiseview・lock screen の上へ行くと、Files は leave も motion も聞かず、最後の item を明るいまま描く。
  P1 の ws127-p002 の手順（item → desktop の隅）は motion を取られない経路なので通った。元の観察（w09）の手順の細部は残っていないので、
  これが原因と断定はできない（候補の 1 つ、見て分かる形の不具合）。
- 直し: `seat.c` に `motion_taken_leave`。zdesktop が motion を取ったら、pointer のある surface に leave を送り `pointer_surface` を NULL に
  する。次に client の motion になれば `zwl_seat_pointer_update` が enter を送り直す（button の配達も先に update する）。窓の move・resize・
  pull・client の move/resize の要求（interactive）・desktop の swipe・drag and drop（data.c が自分で leave する）の間は今までどおり。
- 試験: `plan/ws099/tests/bug141-hover.sh`（item を click → 窓の空き → 別の item に hover（明るい）→ F10 の menu の行へ pointer → その item の
  箱が base.png と同じに戻る）。

## 確かめ

- build: `make … build/p2-p024-img/bin/wayland`（zedBSD の clang、-Werror）exit 0、warning 0。Linux の build の flag で `input.c`・`main.c`・
  `seat.c` を gcc の `-fsyntax-only -Werror` で通した（FreeBSD は未実施）。`style-check.py` は変えた行に違反 0。
- QEMU（T1 に依頼）: bug142-order・bug141-hover と、入力・pointer の回帰（cursor-owner、zdesktop-p077、menu-p003、files-p002、zdesktop-p084）。結果は未着。

## 結果（Q1、2026-10-03、T2-004、QEMU）

uncleared。bug142-order・cursor-owner・zdesktop-p077・menu-p003・files-p004・zdesktop-p084 は PASS。bug141-hover は 2 回 FAIL（`hover: the item is not lit (118 pixels) MISSING`、`menu: the item stays lit (5655 pixels differ) FAIL`）。目視（判定外）では hover-on.png で tile が明るく menu.png で暗く、試験の判定（画素の数え方・領域）の誤りの疑い。PNG: worktrees/t2/build/t2-004/bug141/・bug141-retry/。修正前の対照は未実施。再開の条件: P2 が試験の判定を直し（必要なら修正前の対照も）、T1/T2 で再試験。

## T2-004 の結果（2026-10-03）と試験の判定の直し

- T2-004: bug142-order ほか 6 本 PASS、**bug141-hover は 2 回 FAIL**（`hover: not lit (118 pixels)`、`menu: stays lit (5655 pixels)`）。T2 の目視（判定外）では
  hover-on.png で Plan v3.key の tile が明るく、menu.png で暗い（直しは効いている）。
- 判定の誤り: (1) tile の hover の地は淡い灰（差は 10 段ほど）で、差の閾値 12 では拾えなかった。(2) 4 番目の item は F10 の menu の Help の submenu に
  覆われ、menu.png の差が submenu のものになった。
- 直し（`plan/ws099/tests/bug141-hover.sh`）: hover する item を menu の開く所から遠い 1 番目にし、差の閾値を 3 に、pointer の矢印の所を除き、
  lit は 1500 画素以上・menu は 200 画素以下。T2-004 の PNG で確かめた: 4 番目の tile の hover-on は 7422 画素、1 番目の tile の hover-on（hover していない）
  と menu は 0 画素。

## 再試験（Q1、2026-10-03、T2-008、QEMU）

cleared。f817299bb の直した bug141-hover を T2-004 の image（2217907f6）で: hover 7550 画素で lit、menu で 0 画素、`bug141: PASS`。注: f817299bb の guest の道具の hostmem の既定 1G は 2 GiB の device 窓の前の image では Venus が起動しないため、T2-004 と同じ `VENUS_HOSTMEM=256M` で流した（image の世代の差で、退行ではない）。
