<!-- awesome-plan project=zedbsd record=ws094 -->

# WS094: desktop の file の icon

<!-- awesome-plan-current:start -->
Status: incomplete
Primary Milestone: MG006
Related Milestones: MG006
Objectives: O2
Parent: [Master](../master.md)
Queue: q582 / q582-i01（Agent B2、worktree `/home/awe/zedBSD-worktrees/b2`、branch `codex/b2-ws094`）
Resume point: L3 の計測（p008）は 2026-09-30 に cleared。基準値は (a) 2908 ms（目標 1500）・(b) 1094 ms（2500）・(c) 95 ms（50）・SLOW-FRAME 0。p009 は 2026-09-30 に uncleared: (a) 1954 ms（画面に出るまで 2973）、(c) 90 ms、(b) 1344、SLOW-FRAME 0。残りは zdesktop の import（WS035）と QEMU の Venus の呼び出し 1 回約 10 ms で、進め方は main の判断待ち（phase009 の「残り」）。p013（jpg・gif の thumbnail）は 2026-09-30 に cleared（Image Viewer と共有の decoder `userland/desktop/picture/`）。段の計画は下の「段（L1〜L5）」
<!-- awesome-plan-current:end -->
作業の手引き（2026-10-01）: [guide.md](guide.md)

## 目標（2026-09-29 ユーザー）

「デスクトップにファイルアイコンの表示。」

- `~/Desktop` の file と folder を desktop（壁紙の上）に icon と名前で並べる。double click で開く（WS093 の対応）、選択・移動（配置の保存）・
  右 click の menu・drag and drop（Files との間）、file の増減の追従。compositor（WS035）と Files（WS071）のどちらが描くかは p001 で決める。

## Phase

| Phase | 目的 | Status | 依存 |
| --- | --- | --- | --- |
| [ws094-p001](phase001/phase.md) | 設計（[design.md](design.md)、J1〜J7 は既定: Files が背景の層の client として描く） | cleared（2026-09-29） | — |
| [ws094-p002](phase002/phase.md) | compositor: `keiland_desktop_v1`（role・token・configure）、重ね順・合成・入力・focus・popup・DnD の対象、`--session` での起動と起こし直し、probe | cleared（2026-09-30） | p001、main の許可（WS035 の source） |
| [ws094-p003](phase003/phase.md) | libkeiland の client の API と Files の `--desktop` の骨組み（`~/Desktop` の icon を右上から描く、監視） | cleared（2026-09-30） | p002、main の許可（libkeiland） |
| [ws094-p004](phase004/phase.md) | 選択・開く（WS093）・keyboard・配置の保存と Clean Up | cleared（2026-09-30、2 回目: 保存した場所への配置 PASS、band の撮り直し、回帰（probe・files-open・host）と boot test PASS。probe の試験の順の誤りを直した） | p003 |
| [ws094-p005](phase005/phase.md) | context menu（項目・空いた所）・名前の変更・Trash・Copy・Paste・New Folder・Show in Files | cleared（2026-09-30: menu の手順 PASS、p003・p004 の手順・probe・files-open・host・C9（10 本）・boot test PASS。compositor の menu-shell.c の desktop の menu の閉じ方を直した（main 了解）） | p004 |
| [ws094-p006](phase006/phase.md) | drag（desktop の中・folder へ・Files の窓との DnD）と touch | cleared（2026-09-30: drag・touch の手順 PASS、p003〜p005 の手順・files-open・host・boot test PASS。probe の restart は試験の probe が時々終わらない（下の注）） | p005 |
| [ws094-p008](phase008/phase.md) | L3a: 100 項目の計測の道具と基準値（計測だけ） | cleared（2026-09-30、QEMU。(a) 2908 ms・(b) 1094 ms・(c) 95 ms・SLOW-FRAME 0） | p006 |
| [ws094-p009](phase009/phase.md) | L3b: 100 項目で L3 の数値目標に入れる | uncleared（2026-09-30、QEMU。(a) 1954・(c) 90 ms で超過。残りは zdesktop の import と Venus の 10 ms、main の判断待ち） | p008 |
| [ws094-p013](phase013/phase.md) | L3c: jpg と gif の thumbnail（`fm_image_load` が PPM・PGM・PNG だけ。Image Viewer と同じ libjpeg-compat（EXIF の向き）・libgif-compat（最初の frame）で読む）。100 項目の画像に jpg・gif を混ぜて thumbnail が出る | cleared（2026-09-30、host と QEMU。decoder を `userland/desktop/picture/` に共有） | p009 |
| [ws094-p010](phase010/phase.md) | L4a: 長い名前（2 行、中を省く）と画面の大きさの変更 | cleared（2026-09-30: 統合の試験 demo-s8-s9.sh で合格（ユーザーの指示）、Terminal と p088 は未切り分け、実機は未実施） | p006 |
| [ws094-p011](phase011/phase.md) | L4b: 置き場の溢れ（grid より多い項目）と、無い名前の保存の行の掃除 | in-progress（q582-i01） | p010 |
| ws094-p012 | L5: 実機（5330）での L1〜L3 の確認 | planned | p009、実機 |
| ws094-p007 | 全文の規約と回帰（WS の最後。選んだ段の Phase の後） | planned | 最後の段の Phase |

注（p006）: `desktop-guest.sh` の restart の手順で、試験用の probe（`--timeout-s=3`）が時々終わらず、起動の上限の行が出ないことがある（p002 で 1 回観察、
p006 で 2 回再現、別の 2 回は再現せず）。compositor は role と ack を正しく扱っている。p006 は Files だけを変えており、compositor は p005 と同じ。
probe の側（または libc・kernel の poll や time）の原因は未調査。main に報告済み（bug の起票は main の判断）。

## 段（L1〜L5、2026-09-30 main の方針「広く浅く」）

各段は「まず動く」から磨き込みへ進む。1 つの段を小さな Phase に分け、数値目標と測り方を持つ。確かめるのは QEMU の Venus（L5 だけ実機）。

| 段 | 内容 | 数値目標 | 測り方 | Phase | 状態 |
| --- | --- | --- | --- | --- | --- |
| L1 | icon の表示・開く・選択・menu・名前の変更・Trash・Copy・Paste・New Folder・Clean Up | 手順 show・input・saved・menu が PASS（誤りの log 0） | `files-desktop-guest.sh install show watch input saved menu` | p002〜p005 | 済み |
| L2 | drag（desktop の中・folder へ）、Files の窓との DnD（両方向）、touch（double tap・long press・long press の後の drag） | 手順 drag・touch が PASS | `files-desktop-guest.sh install show drag`（base image）、`… install show touch`（pen image） | p006 | 済み |
| L3 | 項目が多いときの速さ | `~/Desktop` に 100 項目（画像 20 を含む）で、(a) `files --desktop` の起動から `DESKTOP ready` まで 1500 ms 以内、(b) file を 1 つ足してから表示まで 2500 ms 以内（今の監視は 2 秒ごと）、(c) click から選択の frame まで 50 ms 以内（`SLOW-FRAME` の行が 0） | p008 で作る計測の手順（Files の log の時刻と `SLOW-FRAME`、3 回の中央値） | p008（計測）・p009（直し） | 一部（p009、QEMU: (b) と SLOW-FRAME は以内、(a) 1954・(c) 90 ms は超過） |
| L4 | 見た目と端の場合 | (a) 40 文字の名前が 2 行に収まり中を省く（画面で確認）、(b) 1280x800 から 1920x1280 に変えても保存の場所を保ち、外れた項目が空いた cell に入る（log）、(c) grid の cell より多い項目は描かずに数を log、保存の行の無い名前は次の保存で消える | host の試験と guest の画面・log | p010・p011 | 未着手 |
| L5 | 実機 | 実機（5330）で L1 の手順と L3 の (a)(c) が QEMU の目標以内 | 実機の手順（p012 で決める） | p012 | 未着手（実機が要る） |

WS の完了の条件（design §7 の受け入れ）は L1・L2 で満たした。L3 以降は磨き込みで、main が順番と止め時を決める。p007（全文の規約と回帰）は、選んだ最後の段の後に行う。

### 進め方とハーネス（2026-09-30 Q1 の補足）

- **L3 の作業像**: まず計測だけの Phase（p008）で、100 項目の `~/Desktop` を作る script（`plan/tools/files/make-home.sh` を広げ、画像 20 を含む）と、
  `files --desktop` の log の時刻（`DESKTOP ready`、file の追加から表示、`SLOW-FRAME`）を集める script を作り、3 つの数値を出す。直すのは p009 で、
  一番遠い目標から 1 つずつ（例: thumbnail の作成を後回しにして ready を早める）。
- **ハーネス**: 既存の `plan/ws094/tests/files-desktop-guest.sh` に手順 `perf100` を足す。判定は log の時刻だけで行い、画面は確かめの 1 枚。
- L4・L5 は、他の WS が L2・L3 にそろってから。

### ユーザーの判断（2026-09-30、溢れた icon）

- 1280x800 の画面に 100 項目を置くと 13 列 × 7 行の 91 個だけが出る件は「今のまま」（macOS や Windows と同じく、溢れた分は出さない）。L3 の計測は 91 個の表示で行う。

### Q1 の判断（2026-09-30、p009 の再開の条件）

- p009 の残り（(a) 1954 ms・(c) 90 ms）の大部分は Files の外（zdesktop の buffer の import と、QEMU の Venus の同期の呼び出しの約 10 ms の刻み）にある。
- **L3 の (a)(c) の合否は実機（5330、L5 の p012）で判定する。** QEMU の値は参考として記録し、合否に使わない（Venus の 10 ms の刻みは QEMU に固有の見込みのため）。
- zdesktop の import の短縮（`import_layout` の submit と `vkQueueWaitIdle` を次の合成の barrier にまとめる）は、全ての app の最初の frame に効くので、compositor の Phase として WS099 の p016 に移す。
- Venus の 10 ms の調査は Future Work（F-064）に。Files の frame ごとの command buffer の事前の記録は、実機の値を見てから。
- 広く浅くの方針で、WS094 は L3 をここで区切る。次は p013（jpg・gif の thumbnail、デモの写真が汎用の icon になるため）。

## q582 / B2 checkpoint（2026-10-02）

p011を有限Queueで実行中。hidden/prune・失敗listingからの復帰のhost試験とwarning0 target buildがPASS。guest/bootは資源待ちで未実施。WSはincompleteのまま、p007/p012とp009の判定はこのQueueに含めない。[詳細](phase011/q582-result.md)。
