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

（未了。2026-10-02 Q1 の一時停止の指示（host の disk の空けの間、新しい QEMU を始めない）で guest を止めた。再開後に記入。）

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

（未了、guest の再開待ち。）

## 4. 速さの基準値

（未了、guest の再開待ち。）

## 5. 候補の一覧

（3・4 の後に記入。）
