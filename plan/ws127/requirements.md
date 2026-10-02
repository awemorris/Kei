<!-- awesome-plan project=zedbsd record=ws127-requirements -->

# WS127 Files の棚卸し（ws127-p001、q595-i01）

2026-10-02、P4（worktree `/home/awe/zedBSD-worktrees/p4`、branch `agent/p4`、main 08405114a に合わせてから）。product の source は変えていない。
QEMU の証拠だけ（実機は未実施）。console・serial の log は判定に使っていない（判定は Files・zdesktop の log の行と VNC の画面）。

## 1. 回帰の基準（F1）

### image

- `sh plan/tools/files/build-files-image.sh build/p4-files`（main 08405114a、`plan/tools/files/config-amd64-files.mk`）→ exit 0。
  `build/p4-files/hdd-image.img` SHA-256 `fd9ab55625b4c4bebc2330b4e3e9c05c1a57b4b44456ab6333b19eae1bb34440`（2,216,689,664 byte）。
  build の warning は外部 package（openssh）と perl の locale の物だけ。

### host の試験

| 試験 | command | 結果 |
| --- | --- | --- |
| files-model | `timeout 900 sh plan/tools/files/host-model.sh` | `files-model: PASS`（ok 101 行） |
| libz-compat・libpng-compat | `timeout 900 sh plan/tools/files/host-png.sh` | `host-png: PASS` |
| Always Open With | `timeout 900 sh plan/tools/files/host-default.sh` | `host-default: PASS`（ok 21 行） |

### guest の試験（`files-regress.sh` の 14 本）

guest: `GUEST_RUNTIME=build/p4-run sh plan/tools/files/files-guest.sh start build/p4-files/hdd-image.img`（Venus、lavapipe、KVM）。
1 回目 `timeout 7000 sh plan/tools/files/files-regress.sh build/p4-files-out/regress`（host の load average 9〜30、他の担当の build・QEMU と同時）、
2 回目は FAIL の 7 本だけ `files-regress.sh build/p4-files-out/rerun p002 p003 p005 p012 p008 p014 p017`（load 6〜9）。

| 試験 | 1 回目 | 2 回目 | 分類 |
| --- | --- | --- | --- |
| p002 窓・移動 | FAIL（窓の位置を読む前に grep: `surface 0 at 0,0`、click がずれる） | PASS | 試験の時間の余裕（負荷） |
| p003 選択・list・並べ替え | FAIL（同じく位置 0,0、SELECT・SORT の行なし） | FAIL（`SELECT count=3` だけ） | 試験の時間の余裕の疑い（落ちる check が毎回違う）。未確定 |
| p004 file の操作 | PASS | — | — |
| p005 tag・検索・favorite | FAIL（FAVORITE add/remove） | FAIL（TAG・SEARCH・Recents・FAVORITE） | 同上、未確定 |
| p006 ホーム | PASS | — | — |
| p007 preview・Quick Look | PASS | — | — |
| p012 開く・情報 | FAIL（double click が 2 回の click になり OPEN なし、Quick Look の OPEN 行は check の後に出た） | PASS | 試験の時間の余裕（負荷） |
| p008 menu | FAIL（TAG） | FAIL（INFO・TAG・back） | 未確定（落ちる check が毎回違う） |
| p014 titlebar の操作 | FAIL（多数、SSH の banner の timeout を含む） | FAIL（`GLASS undock` だけ） | **試験の座標が古い**（下） |
| p013 tab | PASS | — | — |
| p015 glass | PASS | — | — |
| p009 context menu | PASS | — | — |
| p017 glass と dock | FAIL（`GLASS undock`、restored cards） | FAIL（同じ） | **試験の座標が古い**（下） |
| p010 PNG・DnD | PASS | — | — |

- 基準: 1 回目 7/14 PASS、FAIL の再試験で 9/14。残る 5 本のうち p014・p017 は試験の側の誤り（dock した bar の (190,17) を double click して undock を待つが、
  今の dock の bar は「Files」の名前・戻る・進むの control が並び (190,17) は **進む** の button。手で確かめた: 名前の所 (120,17) の double click で
  `ZWL GLASS undock surface=8 via=double-click` が出て戻る。product の誤りではない）。p003・p005・p008 は落ちる check が回ごとに変わり、同じ時間に
  Files の log に present が 0.4〜2.6 秒の SLOW-FRAME が出ている（host の負荷の時の QEMU の Venus の present の遅れ）。timing の不安定と見るが、
  負荷の低い時の再試験をしていないので未確定。log と画面: worktree の `build/p4-files-out/regress*`・`rerun*`。

## 2. spec.md（WS071）との照合

判定: 済み・部分・無し。根拠の source は `userland/desktop/files/`、試験は `plan/tools/files/`。「設計で変更」は WS071 の design.md の決めによる差。

| § | 題 | 判定 | 根拠 | 足りない物（Future Work） |
| --- | --- | --- | --- | --- |
| 1 | 概要 | 部分 | sidebar・titlebar の CONTROLS・パンくず・icon/list・preview・tag・最近・Quick Look は有る。glass の card（`glass.c`）、System Menu（`menu.c`） | クラウド・共有（F-032） |
| 2 | 窓の構成 | 済み | 浮く titlebar の CONTROLS（`titlebar.c`）、最大化で system bar に入る（`wayland/titlebar-shell.c`）。files-p014・p017 | — |
| 3 | 上の toolbar | 部分（設計で変更） | Back・Forward・Home・path・検索・Icons/List・Preview・進捗（`titlebar.c:43-52`）。窓の button は zdesktop の titlebar（design §3） | toolbar に並べ替え・表示 option の control が無い（View > Sort By・context menu・list の見出しだけ）。FW なし |
| 4 | 戻る/進む | 済み | tab ごとの履歴（`ui.c` fm_ui_back・fm_ui_forward）。files-p002・host-p014 | 長押し・右 click の履歴の一覧（spec でも任意） |
| 5 | ホーム | 済み | dashboard（`ui-home.c`）。files-p006 | — |
| 6 | パンくず | 済み | `ui.c` fm_ui_crumbs、各部分が click できる、幅が足りなければ「…」（`titlebar-shell.c`）。files-p014 | — |
| 7 | 検索 | 部分 | 名前・拡張子・`tag:`・`kind:`（`search.c`）、入力の 150 ms 後に検索、範囲 This Folder/Home/Computer（`ui-search.c`）。files-p005・p014 | 中身の検索（F-034）。metadata は kind と tag だけ |
| 8 | 左 sidebar | 部分 | Favorites・Locations（Recents・Trash・Computer・mount）・Tags、icon 付き（`places.c`）。files-p002・p005 | クラウドの節（F-032）、「共有」の項目（FW なし） |
| 9 | sidebar の編集 | 済み | drag で並べ替え、Remove from Sidebar、folder を Favorites に落とすと追加、Ctrl+Alt+T。host-p010・files-p010・p005 | ピン留め＝Favorites への追加 |
| 10 | 本体の表示 | 済み | icon（`ui-grid.c`）・list（`ui-list.c`）。files-p003 | カラム・ギャラリー（spec でも将来、F-033。menu の項目は無効で有る） |
| 11 | icon の表示 | 部分 | folder は項目数、file は大きさ。thumbnail は PPM・PGM・PNG・JPEG・GIF（`thumb.c`）。files-p007・p010・host-png | 動画・PDF の thumbnail、disk の cache（F-035） |
| 12 | list の表示 | 部分（設計で変更） | Name・Kind・Size・Date Modified、View > List Columns で Changed・Tags・Owner（`menu.c:180-185`）、見出しの click で並べ替え。files-p003 | 作成日時は FS に birth time が無いので Changed（ctime）（design §3.2） |
| 13 | 選択 | 済み | click・Ctrl+click・Shift+click（`select.c`）、矩形（`ui-input.c`）、accent の色。files-p003・files-lag | — |
| 14 | double click/Enter | 済み | folder は開く、file は既定の app（`apps.c`、WS093）。files-p012・files-open.sh | — |
| 15 | 右 click の menu | 部分 | System Menu の context menu（WS070 v2、`menu.c`）: Open・Open With・Cut・Copy・Paste・Rename・Duplicate・Tags・Get Info・Move to Trash（`ui-context.c`）。files-p009・host-p009 | 「移動」（Move To ▸）・「共有」が無い（design §10.2 で計画、未実装）。FW なし |
| 16 | DnD | 済み | 窓の中: folder・sidebar・tag・Trash・tab へ。Ctrl で copy、Ctrl+Shift で link、ask の menu（Move/Copy/Link Here）。他の窓・app へ text/uri-list（`dnd.c`）。host-p010・files-p010 | 窓の外では move/copy だけ（zdesktop が決める）。自動の scroll・spring-loaded は F-039 |
| 17 | preview | 部分 | thumbnail・名前・Kind・Size・Modified・Where・Tags・text の最初の 12 行（`ui-preview.c`）。Ctrl+Alt+P か Preview の control。files-p007・p014 | PDF・動画・音声は icon だけ（F-035） |
| 18 | Quick Look | 部分（設計で変更） | Space で窓の中の overlay、←→ で次、Esc で閉じる（`ui-preview.c`）。files-p007 | spec は system の preview。design §9 で窓の中に（F-040） |
| 19 | 最近のファイル | 済み | app を横断する recent（`libkeiland/recent.c`、textedit・imageview・pdfviewer・notes も使う）。files-p006・p005 | — |
| 20 | タグ | 部分 | 1 file に複数、xattr `user.keiland.tags`、名前と色（`tags.c`）、sidebar で絞り込み。files-p005・p008 | タグの icon（spec の「任意のアイコン」）。FW なし |
| 21 | file の情報 | 部分（設計で変更） | Where・Kind・MIME type・Size・Modified・Changed・Accessed・Owner/Group・Permissions・Links・Tags・Extended attributes（`ui-info.c`）、SHA-256 を背景の task で（`info.c`）。files-p012 | 作成日時は「—」（design §9）。フルパスは Where と名前に分かれる |
| 22 | 名前の変更 | 済み | F2・context menu、stem を選んだ inline の編集（`actions.c` fm_action_rename_begin）。files-p004 | 拡張子を選択から外すかの設定は無い（常に外す） |
| 23 | 削除 | 済み | Delete で Trash、Shift+Delete は確認の dialog の後に削除（`ui-input.c`・`actions.c`・`ui-overlay.c`）。files-p004 | — |
| 24 | ゴミ箱 | 済み | freedesktop.org の Trash（`trash.c`）、元の場所・削除日の列、Put Back・Delete Immediately・Empty Trash。files-p004・host-p009 | `$topdir/.Trash-$uid`（F-041） |
| 25 | 装置/mount | 部分 | mount 中の実の volume を Locations に（`places.c`、`mntent/`・`freebsd/`） | 未 mount の表示、eject（F-036）、NAS・SMB・WebDAV（F-032） |
| 26 | network 共有 | 無し | Go > Network は無効（`menu.c`） | 「サーバーへ接続」、smb・nfs・dav（F-032） |
| 27 | クラウド | 無し | — | 同期の状態（F-032） |
| 28 | ホームの dashboard | 部分（設計で変更） | hero・folder の card（Projects・Downloads を含む）・Recent Files・Recent Folders（`ui-home.c`）。files-p006 | 「おすすめ/ピン留め」は sidebar の Favorites に（design §3.1） |
| 29 | 複数の窓 | 部分 | Ctrl+N で新しい process（`main.c`）。files-p008 | folder の context menu の「新しいウィンドウで開く」が無い（Open in New Tab だけ）。FW なし |
| 30 | タブ | 済み | Ctrl+T・Ctrl+W・次/前（`ui-tabs.c`）、2 つ以上で tab の列。files-p013・host-p013・p015 | — |
| 31 | Undo | 済み | move・copy・trash・restore・rename・新しい folder・tag（`ops.h`、`actions.c` fm_action_undo）、Redo は Ctrl+Shift+Z。files-p004 | — |
| 32 | 状態の表示 | 済み | 要る時だけの pill（選択数と大きさ、copy 中、message。`ui-grid.c` grid_status） | — |
| 33 | 進捗 | 済み | 背景の task（`task.c`）、titlebar の進捗の輪から task の一覧と cancel（`ui-titlebar.c`・`ui-overlay.c`）。host-p014 | — |
| 34 | 検索の索引 | 無し | — | system の indexer、中身の索引（F-034） |
| 35 | keyboard | 済み | spec の 16 の key が全て有る（`ui-input.c` の表、`menu.c` の shortcut）。files-p003・p004・p008・p013・p014 | — |
| 36 | system menu | 部分（設計で変更） | File・Edit・View・Go・Window・Help を System Menu で（`menu.c`）、dock の時も。files-p008・p014・p017 | CONTROLS の時 menubar は titlebar の「…」の popup（design §3、WS070 の決め A） |
| 37 | menu の例 | 部分 | 例の項目は全て有る（`menu.c:98-165`） | Columns・Gallery は無効（F-033）、Network は無効（F-032） |
| 38 | 見た目 | 済み | button は少なく詳細は menu、glass の card と読める本文、色は選択と tag、thumbnail。files-p015・p017 | 主観の評価はユーザーの目視で |

集計: 済み 19、部分 16（うち設計で変更 6: §3・§12・§18・§21・§28・§36）、無し 3（§26・§27・§34）。
FW の無い差: 「移動」・「共有」の context menu（§15）、「新しいウィンドウで開く」（§29）、sidebar の「共有」（§8）、タグの icon（§20）、toolbar の並べ替えの control（§3）。

（照合は source の読みによる。読みの補助に read-only の調べの agent を使い、主な主張（context menu の項目、Columns・Gallery・Network の無効、タグの定義に icon が無い）を source で確かめた。）

## 3. 実使用の通し

道具: `plan/ws127/tests/walk-lib.sh`（zdesktop と files の起動、QMP の pointer と key、VNC の画面、Files の log の grep）。sample home は
`make-home.sh`、それに guest の中で 1000 項目の folder `Big`（txt・csv・folder・bin を 250 ずつ）と、host で作った jpg・png・gif の 12 枚の `Photos` を足した。
画面は worktree の `build/p4-files-out/walk/w01〜w13.png`、代表 8 枚を `plan/ws127/tests/evidence/*.jpg` に縮めて保存。

| 操作 | 結果 | 根拠 |
| --- | --- | --- |
| 名前の衝突（Budget.csv を Downloads へ copy） | dialog（Skip・Keep Both・Replace）が出る。Keep Both → `Budget 2.csv`、Skip → 何も copy しない（files=0）、Replace → 古い方は **Trash へ**（中身 old、trashinfo 付き）、Ctrl+Z で古い方が戻り、Ctrl+Shift+Z で再び置き換え | `COLLISION answer=keep/skip/replace`、`TASK done … files=2`、guest の file の中身。w02 |
| Trash・Empty | Trash の一覧、Empty Trash は確認の dialog（Empty・Cancel）、Enter で 2 項目を削除、Trash の files と info が空 | `DIALOG ask=2`、`TASK done kind=delete files=2`。w04・w05 |
| undo/redo | 上の Replace と Put Back で確かめた | `TASK done kind=restore` |
| 1000 項目の folder | 開ける（`items=1000`）、icon の grid、Favorites に足せる（Ctrl+Alt+T） | w06 |
| 画像の多い folder | jpg・png・gif の thumbnail が全て作られる（256 px、`THUMB error=0` ×12） | w09 |
| 長い名前・日本語の名前 | 長い名前は 2 行と省略「…」、`会議メモ.txt` は表示される | w01。日本語の名前への**名前の変更**はこの image に IME が無いので未実施 |
| New Window（Ctrl+N） | 2 つ目の process と窓（+96,+9 にずらして） | `ZWL MAP client=2`、w10 |
| 窓の間の DnD | 窓 2 の photo00.jpg を窓 1 の sidebar の Big へ落とす → move | `DND dropped`、`DROP operation=move`、`TASK done kind=move`、file が Big に。w11 |
| dock と undock | 浮いた titlebar の double click で dock、dock の bar の「Files」の名前の double click で戻る | `GLASS dock/undock`。w12・w13 |
| 検索・tag・preview・Quick Look・tab・rename・新しい folder・context menu・窓の中の DnD | 通しでは行わず、回帰の p004・p007・p009・p010・p013（PASS）と p005（1 回目は TAG・SEARCH が ok）で代える | 1 の表 |
| Open With・Always Open With | **未実施**（`files-open.sh` は worktree の binary を guest に送る形で、この Queue の時間では行わなかった） | — |

### 見つけた不具合

重い（データを失う・止まる）は 0。

| # | 重さ | 症状 | 再現 | 見込みの所 |
| --- | --- | --- | --- | --- |
| B1 | 中 | Trash の一覧で、同じ名前の 2 つ目が Trash の中の file の名前 `Budget.csv.2` と kind「2」で出る（元の名前 `Budget.csv`・CSV で出るべき） | 同じ名前の file を 2 回 Trash へ（今回は Replace の後の undo/redo）。w05 | `dir.c` の Trash の読み（trashinfo の Path から表示の名前と kind を作っていない） |
| B2 | 中（QEMU だけで見た） | 起動直後に present が 2.6 秒止まった間に押した Ctrl+C（System Menu の shortcut、`MENU item=1010 action=10`）が、次の sidebar の click の**後**に届いて処理され、選択が無くなっていたので copy されず、続く Ctrl+V は「Nothing to paste」 | 起動直後に select → Ctrl+C → すぐ別の folder へ移動。負荷の高い host の QEMU で 1 回。実機は未確認 | menu の action と pointer の event の順序（zdesktop の menu の経路と files の event loop の present の待ち） |
| B3 | 軽い | pointer が窓の外へ出ても、最後に指した item の hover の背景が残る | item を click して pointer を desktop の隅へ。w09 の photo01 | `ui-input.c` の pointer の leave |
| T1 | 試験 | p014・p017 の undock の座標が今の dock の bar の配置と合わない（上） | 毎回 | `plan/tools/files/files-p014.sh:157`・`files-p017.sh:137` |
| T2 | 試験 | p002・p003・p005・p008・p012 が host の負荷の時に落ちる（窓の位置を読む前の grep、double click の間隔、check の待ち） | 負荷の時 | `plan/tools/files/files-p0*.sh` の待ち |

重い・中（B1・B2）は Bug Board の候補として Q1 に渡す。

### 計画への影響

- **F-050 は済んでいる**: Replace で置き換えた item は Trash へ行き undo で戻る（今回確かめた）。Future Work の表では F-050 は ws035-p110・p115 へ
  promoted 済み（cut の Esc の clipboard、folder の merge も同じ）。**ws127-p003 の (1)〜(3) は実装済み**で、残りは (4) home の外の volume の
  `$topdir/.Trash-$uid`（`trash.c` に無い）だけ。

## 4. 速さの基準値（QEMU の値。実機ではない）

QEMU の Venus（lavapipe が host の CPU で描く、egl-headless が scanout を読み返す）、KVM、host の load average 6〜9。Files の log には時刻が無いので、
(a) は guest の時計（`date +%s%N`）で process の起動から `ZFILES READY`（最初の frame を present した直後に出る）まで、(b)(c) は host の
`plan/ws127/tests/latency.py`（QMP の button press から、VNC で 2x2 の点を繰り返し読んで期待の色になるまで。QEMU の表示の更新と RFB の往復を含む上限）。

| 項目 | 値（ms） | 中央値 |
| --- | --- | --- |
| (a) 起動 → 最初の frame、1000 項目の folder | 1756・1794・2544 | **1794** |
| (a) 同、6 項目（Documents） | 2449・1692・3002 | 2449 |
| (a) 同、画像 12 枚（Photos） | 2875・2676・2577 | 2676 |
| (b) 窓の中で sidebar の click → 1000 項目の icon が画面に出る | 927・660・576・555・563 | **576** |
| (b) 同、6 項目の folder へ | 405・395・499 | 405 |
| (c) click → 選択の色が画面に出る、1000 項目の folder | 245・248・273・378・303 | **273** |
| (c) 同、6 項目の folder | 254・282・413・287・218 | 282 |

- 項目の数はほぼ効かない（(a) は起動の固定の費用、(c) は 1000 でも 6 でも同じ）。遅い frame の内訳（`SLOW-FRAME`）は draw 13〜28 ms・present 406〜2585 ms
  （queue が大半）で、CPU の描画（F-037 の damage の矩形で減る所）より QEMU の Venus の present の待ちが支配的。
- F4 の仮の目標（1000 項目を開いて ≤1000 ms、選択 ≤50 ms）に対して、QEMU では開くのは満たし（576 ms）、選択は満たさない（273 ms）。QEMU の present の経路が
  支配的なので、目標の判定は実機（5330、ws127-p007）で行うのがよい。

## 5. 候補の一覧（ユーザーが採否を選ぶ）

目安は 1 Phase の実装＋回帰の時間。衝突する file は `userland/desktop/files/` の下（WS127 の実装の Phase は互いに直列）。推奨は P4 の案。

| # | 項目 | 価値 | 目安 | 危険 | 依存 | 触る file | 推奨 |
| --- | --- | --- | --- | --- | --- | --- | --- |
| 1 | B1 Trash の元の名前と kind の表示 | Trash の中身が分かる（中の不具合） | 1h | 低 | — | `dir.c`（Trash の読み）、`ui-grid.c`・`ui-list.c` の名前 | 入れる（p002） |
| 2 | B2 起動直後の shortcut の順序の調べと直し | copy が黙って失われない | 2〜3h（実機で再現するかの確認を含む） | 中（zdesktop の menu の経路に及ぶかも） | 実機で再現するか（p007 と一緒に） | `main.c`・`present.c`、場合により `wayland/` の menu（P2・P3 と調整） | 調べは入れる（p002）。直しは再現した時 |
| 3 | B3 pointer の leave で hover を消す | 見た目 | 0.5h | 低 | — | `ui-input.c` | 入れる（p002） |
| 4 | T1・T2 回帰の試験の直し（p014・p017 の座標、待ちを log の行で待つ形に） | F1（14 本 PASS）を安定して判定できる | 2h | 低（試験だけ） | — | `plan/tools/files/files-p0*.sh` | 入れる（p002 か p008 の前） |
| 5 | F-041 の残り `$topdir/.Trash-$uid`（旧 p003 の (4)） | USB 等の volume の file を Trash へ | 2h | 中（別の FS の rename） | mount の一覧 | `trash.c`・`actions.c` | 任意（USB を使う人が増えたら） |
| 6 | F-035 PDF の 1 頁目の thumbnail と disk の cache（p004） | 書類の folder の見やすさ、大きい画像の folder の再表示 | 3h | 中（libpdf の描画の費用） | libpdf、`picture/` を変えるなら WS128 と直列 | `thumb.c`・`peek.c` | 入れる（見た目の価値が高い） |
| 7 | F-041 の残り 日本語の UI と rename の IME（p005） | 日本語の利用者 | 4h | 中（Settings と共通の仕組み） | WS095（IME、実機で日本語入力は確認済み）、WS089 p016 | 文言の所・`ui-field.c` | ユーザーの判断（Settings と揃えて） |
| 8 | F-039 DnD の自動の scroll と spring-loaded（p006） | 1000 項目の folder の中で落とせる | 2h | 低 | — | `ui-drag.c`・`dnd.c`・`ui-grid.c`・`ui-list.c`・`ui-tabs.c` | 入れる |
| 9 | F-037 描き直しを damage の矩形に（p007 の条件付き） | 実機で遅い時 | 2〜4h | 中 | 実機の計測（今回の QEMU では draw は 13〜28 ms で、遅さは present） | `present.c`・`canvas.c`・`main.c` | 実機で目標に届かない時だけ |
| 10 | spec の FW の無い差: context menu の「移動」（Move To ▸）と「新しいウィンドウで開く」 | spec §15・§29 | 2h | 低 | — | `ui-context.c`・`actions.c`・`menu.c` | 入れる（小さく価値がある） |
| 11 | spec の FW の無い差: 「共有」（menu と sidebar）、タグの icon、toolbar の並べ替えの control | spec §8・§15・§20・§3 | 各 2h〜 | 中（共有は先が無い） | 共有の先（network、F-032） | `ui-context.c`・`places.c`・`tags.c`・`titlebar.c` | 入れない（ベータ1 の外） |
| 12 | F-033 カラム・ギャラリーの表示 | Finder らしさ | 4h 以上 | 中 | — | `ui-grid.c`・`ui-list.c`・新しい表示 | 入れない（ベータ1 の外） |
| 13 | F-036 装置の unmount・eject | USB の取り外し | 3h | 中 | USB の storage の hotplug の通知 | `places.c`・mount の仕組み | 入れない（hotplug の後） |
| 14 | F-032 network・クラウド、F-034 indexer、F-040 system の Quick Look | spec §26・§27・§34・§18 | 大 | 高 | network の FS、常駐の索引、compositor | 多数 | 入れない |

WS127 の未決の判断への材料: (1) 上の表。(2) libkeiui への移行（WS090 p009・p010）は、上の 1〜4・6・8・10 がどれも `files/` の今の部品の中で閉じるので、
ベータ1 の後に回しても妨げにならない。(3) 日本語の UI は 7。(4) F4 の数値は、QEMU では present が支配的で選択が 273 ms なので、目標は実機（p007）で決めるのがよい。
