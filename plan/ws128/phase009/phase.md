<!-- awesome-plan project=zedbsd record=ws128-p009 -->

# ws128-p009: Terminal の「CJK Ambiguous Width を全角で扱う」の切替

Status: cleared（Q1 判定 2026-10-03: T1 の terminal-p009-guest PASS・menu-p003 PASS（QEMU、main d169912dd）で A7 の Terminal の既存の回帰を満たした）。元の記載: in-progress（q633-i02、P2 generation3 がラップアップ。再開待ち）
Disposition: normal
Parent: [WS128](../ws.md)
Queue: q633
依存: なし。WS131 p018（Terminal の新 API への移行）・ws128-p006 と同時に流さない（同じ `userland/desktop/terminal/`）
目安: 2〜3h（1 Queue）。実行者の目安: phase-runner
所有 path: `userland/desktop/terminal/`、`plan/ws128/`

## 由来

2026-10-03 user:「Terminal appで、Treat "CJK Ambiguous Width" characters as wide というオプションを、メニューで瞬時に切り替えられるようにしてください。これがないと、日本語話者はターミナルでUnicodeで生活するのがつらいです。ad-hocなのは理解できますが、mlterm, mintty, iTerm2など、ほぼすべての実用的なターミナルがこれを持っています。GNOME Terminalはバージョンによってまちまちて、KDE Konsoleは持っていないです。なので日本語話者でEmacsでSKKを使う人は、このオプションがあるターミナルを使っているという現状があります。」

## 目的と受け入れ

| # | 条件 | 確かめ方 |
| --- | --- | --- |
| A1 | View の menu に checkbox「Treat Ambiguous-Width Characters as Wide」があり、選ぶと即座に切り替わる（再起動・新しい tab が要らない） | QEMU の Venus の PNG、Terminal の log |
| A2 | 有効の間、Unicode の East Asian Width が A（Ambiguous）の文字（例: ○ ■ ※ α ё ① ─ …）は 2 cell を占め、cursor も 2 進む。無効なら 1 cell（今の挙動）。結合文字（Mn・Me）と制御は 0 のまま | host の単体試験（`screen_wide` の表を全 code point で）、guest の PNG |
| A3 | 2 cell の glyph が 2 cell の枠で描かれる（左半分だけ・重なりが無い）。罫線（U+2500 台）は有効時に 2 cell の幅で途切れずにつながる | PNG |
| A4 | 設定は Terminal の設定として保存され、次の起動でも保たれる。全ての窓・tab に効く | guest で切り替え → 再起動 |
| A5 | 切替の前に画面にある行は、そのまま再描画で崩れない（既に置いた cell の幅は変えない。新しく出力される文字から新しい幅。mlterm・mintty と同じ扱い）。切替は menu の checked の表示に反映 | PNG |
| A6 | 表は Unicode の EastAsianWidth.txt（版を記録）から生成し、生成の script を残す。範囲の二分探索で、出力の速さが落ちない | host の試験、`cat` で大きな file の時間の比較 |
| A7 | 規約（coding-style の全文）、warning 0、Terminal の既存の回帰、boot test | build・試験 |

## 設計の要点

- `screen.c` の `screen_wide()` に Ambiguous の表を足し、`struct terminal_screen`（または terminal 全体）の flag で切り替える。
- menu は `menu.c` の View に `KEILAND_MENU_ITEM_CHECKBOX` を足し、action を足す。checked の状態は既存の Fullscreen と同じ更新の経路。
- 保存は Terminal の既存の設定の保存の仕組み（無ければ `~/.config` の下の Terminal の file）。新しい OS の依存は持たない。
- 範囲外（残件として記録）: shell・vim・Emacs など中で動く program は自分で幅を計算する。libc の `wcwidth()` は今の Unicode の既定（A は 1）のまま。vim は `set ambiwidth=double`、Emacs は `(set-language-environment "Japanese")` 等の `cjk-ambiguous-width` の設定を使う旨を Terminal の help（または phase の記録）に書く。libc や locale で切り替える案は Future Work の候補として Q1 に返す。

## 実行の記録（q633-i01、P2 generation2、2026-10-03）

状態: 実装と host・guest の確認は済み。残り（下）があるので未 clear。P2 generation2 は Q1 の指示（menu-p003.sh の権限の再起動）で安全な区切りでラップアップし、generation3 が続ける。

### 発見（重要）

- 変更の前の Terminal は、全ての glyph を **1 cell の slot に切り詰め**、font は JetBrains Mono（keiland-mono）だけだった。JetBrains Mono には CJK の glyph が無いので、**日本語は .notdef の箱が左半分で切れて**表示されていた（wide の判定はあったが描画が 1 cell）。A3 のために、2 cell の atlas の slot（同じ行に並ぶ 2 slot）と、fallback の face（既に image にある `/usr/share/fonts/keiland-fallback.ttf`、DroidSansFallbackFull。textedit・files と同じ）を足した（Q1 承認 2026-10-03）。
- 入っている font のどれにも ※（U+203B）と ①（U+2460）の glyph が無い（JetBrains Mono・DroidSansFallbackFull とも）。幅は正しく 2 cell になるが、字は .notdef の箱。font の追加は範囲外（残件）。
- Terminal には結合文字（Mn・Me）を 0 幅にする処理が元から無く、1 cell の自分の cell に描く（今の挙動）。A の表から Mn・Me・Cf・Cc を除いたので、有効にしても結合文字・format 文字は 2 cell にならない（A2 の「0 のまま」はこの意味で満たす。0 幅の結合の描画は範囲外）。

### 設計（実装した物）

- `width.c`/`width.h`: `terminal_width_wide(cp, ambiguous_wide)`。W/F の判定は元の `screen_wide()` の範囲をそのまま移した（off の挙動は全 code point で不変を host で確認）。A は `ambiguous.h`（176 範囲、138370 code point）の二分探索、U+00A1 未満は即 0。
- `ambiguous-gen.py`: unicode.org の Unicode **17.0.0** の `EastAsianWidth.txt`（sha256 `ea7ce50f3444a050333448dffef1cadd9325af55cbb764b4a2280faf52170a33`）と `UnicodeData.txt`（sha256 `2e1efc1dcb59c575eedf5ccae60f95229f706ee6d031835247d843c11d96470c`）から生成（`--fetch` で取得し hash を照合。data は commit しない）。PUA（Co）は Unicode の通り A に含む。
- `screen.c`: `screen_put` が `screen->ambiguous_wide` で幅を決め、cell に固定する。`terminal_screen_set_ambiguous_wide()`。既に置いた cell・scrollback は変えない（A5）。
- `font.c`: `terminal_font_wide_slot()`（key は code point | `0x80000000`、行末の slot は飛ばす、満杯なら slot 1・2 に予約した 2 cell の U+FFFD）。`terminal_font_fallback()`。2 cell の frame では、罫線・block（U+2500〜259F）は横に 2 倍（pixel を 2 回）で端から端まで、他の狭い glyph は中央に置く。
- `render.c`: 右半分が continuation の cell は 2 cell の quad 1 つで描き、右半分を別に描かない。cursor・選択は両半分を見る。
- `menu.c`: View の Fullscreen の下に checkbox「Treat Ambiguous-Width Characters as Wide」（item 36、action 21）。menu は 31 item。
- `settings.c`: **Terminal 自身の file** `~/.config/keiland/terminal.conf`（`ambiguous-wide=0|1`、他の行は保つ、`.new` に書いて fsync・rename）。同じ process の全ての tab（と Reset Terminal の後）は即座に従い、別の process（New Window を含む）は起動時に読む。
  - requirements.md の C4 は `keiland_preferences_*`（desktop.conf）を挙げていたが、Q1 の指示（2026-10-03）で使わない: WS131 p011 で desktop.conf の書き手を compositor だけにし、毎秒の stat の監視を除くため、app の設定を足すと移行の対象が増える。
- `main.c`: 起動時に設定を読む（`ZTERM SETTINGS ... ambiguous_wide=N`）、`--fallback-font=PATH`、action で全 tab を切替・保存（`ZTERM AMBIGUOUS run=... wide=N saved=ERRNO`）、menu の state に `ambiguous_wide=N`。
- 既定は off（今の挙動）。

### 確認（行った物）

| 確認 | 結果 |
| --- | --- |
| host `plan/ws128/tests/terminal-p009.sh build/ws128-p009`（0〜10FFFF の全 code point を off/on で Unicode 17 の data と照合、生成物と `ambiguous.h` の一致、grid の幅・cursor・A5、terminal.conf の読み書きと他行の保持、45〜53 MB の出力の速さ） | PASS。off は 173339 wide で p009 前と同一、on は +138362。速さ on/off 比 1.028・0.959（2 回）。`evidence/host-speed.txt` |
| zedBSD amd64 の image（`plan/ws089/tests/build-settings-image.sh build/p2-p009-img`、`-Wall -Wextra -Werror`） | warning 0、image の check OK |
| host gcc（Linux build の flag、`-Werror`）で terminal の全 source の compile | 0 warning |
| QEMU Venus guest `plan/ws128/tests/terminal-p009-guest.sh`（commit 後の image） | PASS（log 16 項目）。PNG: `evidence/off.png`（既定 off）・`view-off.png`・`on.png`（切替の前の行はそのまま、新しい行は A が 2 cell・罫線の箱が日本語と揃う）・`view-on.png`（checkbox が checked）・`tab.png`（新しい tab も wide）・`restart.png`（終了→再起動で wide を保持）・`off-again.png`（再起動後の窓から off、保存 0）。log `evidence/terminal-guest.log` |

### 残り（generation2 の時点。generation3 の結果は下）

1. `plan/tools/titlebar/menu-p003.sh` の 88・89 行の `items=30` → `items=31`（Q1 委任、user 承認済み 2026-10-03。generation2 は権限で止まり未変更）。直したら menu-p003（Terminal の既存の回帰）を guest で流す。
2. boot test（`plan/tools/boot-test.sh`）。
3. 変えた source の全文規約の review の最終確認（A7）。
4. 範囲外の残件の記録: 中で動く program は自分で幅を計算する。libc の `wcwidth()` は Unicode の既定（A は 1）のまま。vim は `set ambiwidth=double`、Emacs は `(set-language-environment "Japanese")` 等で `cjk-ambiguous-chars-are-wide`（または `char-width-table` の設定）を使う。libc・locale で切り替える案は Future Work の候補として Q1 へ。※・① の glyph（font の追加）も候補。実機は未実施。

## 実行の記録（q633-i02、P2 generation3、2026-10-03）

状態: 未 clear。user の指示（2026-10-03「ホストの仮想メモリの状態のせいで、テストが実行できなくなっていると判断しました。すべてのエージェントをラップアップさせて…」）で、menu-p003 の 3 回目の途中で止めてラップアップした。host の再起動の後に再開する。

### commit（base 73e8b342e）

| SHA | 内容 |
| --- | --- |
| 0564ff2d9 | `plan/tools/titlebar/menu-p003.sh` の 88・89 行を `items=31` に（user 承認の 2 行だけ） |
| 2522cda9b | A7 の全文規約の review の修正（意味は不変）: `width.c`（二分探索の段落の comment、`width_east_asian_wide` の範囲を 1 つの if ずつ comment 付きに）、`settings.c`（snprintf・mkdir・path・read の段落を分け、入れ子の if に comment、`getpwuid(getuid())` の入れ子の呼出を分離、read・write の loop の comment、close と成功の return を分離）、`font.c`（fallback の glyph の呼出・pixel の書き込みの loop・probe の次の place・fallback face の close の comment）、`render.c`（右の cell の取得・cursor の判定の comment）、`menu.c`（header の長い行を折り返し）、`main.c`（`MAIN_FONT_PIXELS` を `MAIN_FONT` の comment の下に戻す、`0U`） |
| （この phase.md の commit） | 記録と evidence |

### 確認（generation3）

| 確認 | 結果 |
| --- | --- |
| A7 規約 review（coding-style の全文、p009 の差分の全体: width.c・settings.c・font.c・render.c・screen.c・menu.c・main.c・terminal.h） | in-scope の違反を 2522cda9b で修正。`git diff --check` OK。既存の code（p009 で触っていない行）の不揃いは範囲外として触らない |
| host `plan/ws128/tests/terminal-p009.sh build/p009/host`（修正の後） | PASS（全 code point の off/on、grid・settings 21 項目、速さ 53 MB on/off 比 1.013） |
| zedBSD amd64 の image `plan/ws089/tests/build-settings-image.sh build/p2-p009-img`（修正の後） | rc 0、warning 0、`check-amd64-native-image: OK` |
| boot test `OUTPUT=build/p009/boot-test plan/tools/boot-test.sh build/p2-p009-img/hdd-image.img` | **PASS**（login prompt）。PNG `evidence/boot-login.png`（QEMU） |
| menu-p003（`GUEST_RUNTIME=build/p2-run`、guest は `zdesktop-guest.sh start build/p2-p009-img/hdd-image.img`）。**3 回とも host の memory の圧迫の下で取った**（user・Q1 の判断、2026-10-03） | 下の 3 行。**未 PASS（未完）** |
| 1 回目 | 全項目 MISSING。compositor の起動が遅く（`ZWL STARTUP step=glyphs ms=5113`、READY は起動の約 52 s 後）、script の 4 s の待ちの後に起動した terminal が `ZTERM FAILED operation=terminal_window_open errno=6`（socket が未だ無い）。harness の待ちの問題で、Terminal の変更とは無関係。出力は 2 回目で上書き |
| 2 回目 | `items=31` の 2 項目・floating の PNG・Edit を開く は ok。Edit > Select All の click が menu を閉じるだけで activate されず（`MENU close ... item=2` の後に activate 無し）、selected.png と Copy の shortcut の 5 項目が連鎖で FAIL。F10 の key・Paste の menu は ok。580 s の timeout で途中終了。`evidence/menu-p003-r2.txt` |
| 3 回目（同じ条件の再試行） | 2 回目に落ちた Select All・Copy を含め 1〜5・7〜10 の項目が ok（items=31、floating/selected PNG、Edit・Copy・F10・Paste の menu・Text Size・Zoom・outside press・dock・docked の menu・fullscreen・New Window の 2 つ目の窓）。`ZTERM PASTE bytes=13` だけ MISSING。2 つ目の窓の後の項目の途中で、ラップアップの指示により止めた（rc 無し）。`evidence/menu-p003-r3.txt` |

判断: 2 回目と 3 回目で落ちた項目が違い（Select All は 3 回目 ok）、どちらも View の新しい item（36）や幅の変更とは関係の無い Edit・Paste の経路。host の memory の圧迫の下の timing の揺れと見るが、確定していない。host の再起動の後に流し直して判定する。

### 再開の手順（host の再起動の後）

1. worktree `/home/awe/zedBSD-worktrees/p2`（branch agent/p2）で main を取り込む。terminal の source を変えていなければ image は `build/p2-p009-img/hdd-image.img`（2026-10-03 11:24、2522cda9b の source）をそのまま使える。変わっていれば `plan/ws089/tests/build-settings-image.sh build/p2-p009-img` で作り直す。
2. `GUEST_RUNTIME=$PWD/build/p2-run plan/ws035/tests/zdesktop-guest.sh start build/p2-p009-img/hdd-image.img`、`GUEST_RUNTIME=$PWD/build/p2-run python3 plan/tools/guest/guest.py wait`。
3. `GUEST_RUNTIME=$PWD/build/p2-run plan/tools/titlebar/menu-p003.sh build/p009/menu-p003`（20 分ほどかかる。timeout は 1200 s 以上に）。全項目 ok なら A7 の「Terminal の既存の回帰」を満たす。`ZTERM PASTE bytes=13` が再び MISSING なら、gdbstub・guest の log（`/tmp/t.log`・`/tmp/zdesktop.log`）で Paste の経路を調べる（p009 の前の terminal の image で同じ手順を流し、p009 の回帰かを切り分ける）。
4. boot test は 2522cda9b の image で PASS 済み。source を変えたら取り直す。

### 範囲外の残件（Q1 へ。Future Work の候補）

- 中で動く program（shell・vim・Emacs）は自分で幅を計算する。libc の `wcwidth()` は Unicode の既定（A は 1）のまま。vim は `set ambiwidth=double`、Emacs は `(set-language-environment "Japanese")` などで `cjk-ambiguous-chars-are-wide`（または `char-width-table`）を使う。libc・locale で切り替える案は Future Work の候補。
- ※（U+203B）・①（U+2460）の glyph が入っている font のどれにも無い（幅は正しく 2 cell、字は .notdef の箱）。font の追加は Future Work の候補。
- 結合文字（Mn・Me）を 0 幅で前の cell に重ねる描画は Terminal に元から無い（範囲外）。
- 実機は未実施（QEMU の証拠だけ）。
