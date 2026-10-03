<!-- awesome-plan project=zedbsd record=ws100-p012 -->
# ws100-p012: 音量を操作のたびに desktop.conf へ書かない（session の終わりに一度だけ、BUG-161）

Status: cleared（2026-10-03、P1。Q1 判定）
Disposition: normal
Parent: [WS100](../ws.md)
Queue: Q1 の割り当て（2026-10-03、P1。Phase の ID は Q1。p011 は WS100 の guide で最終の規約と回帰に予約済みなので飛ばす）
Bug: [BUG-161](../../bugs/BUG-161.md)

2026-10-03 user「音量の変更をいちいちファイルに書くのはおかしいです。I/Oが発生するべきではないです。ログインセッションの終わりに記録する程度でいいはずです。」

## 変えたこと

- `userland/desktop/wayland/volume.c`（system bar）:
  - 操作で file に書かない。`VOLUME_SAVE_MS` と保存の timer を外した。BUG-153 の読み戻しの仕組み（`saved*`・`VOLUME_ECHO_MS`・`zwl_volume_preferences`）も外した。書かないので読み戻しが無い。
  - session の間の音量は audiod だけが持つ。system bar と Settings は audiod の報告に合わせる（今まで通り）。
  - 読み: audiod に最初につながった時に、preferences の `sound.volume`・`sound.muted` を一度だけ読み、audiod に送る（`volume_restore`。log `ZWL VOLUME preferences value=`）。audiod が落ちて再びつながった時は、file ではなく session の音量を送る（`ZWL VOLUME restored … from=session`）。
  - 書き: 新しい `zwl_volume_keep(server, why)`。session の終わりに一度だけ、file にある値と違う時だけ 2 つの key を書く（log `ZWL VOLUME kept value= muted= why= write=0|1`）。呼ぶ所は 2 つ:
    - Log Out（`handoff.c` の `zwl_handoff_logout`、`why=logout`）
    - zdesktop の終了の片付け（`main.c`、preferences を閉じる前、`why=end`）。QUIT・SIGTERM（電源断）の両方の終わりに当たる。
  - preferences の無い greeter は書かない。
- `userland/desktop/wayland/preferences.c`: file の変化で音量を適用しない（comment で理由を残した）。
- `userland/desktop/settings/sound.c`: slider を離した時の `sound_save`（desktop.conf への書き）を外した。page の初めの表示は今まで通り file の値（audiod の報告で置き換わる）。
- 試験:
  - `plan/ws100/tests/volume-p004.sh` の A5:
    - session の始めの desktop.conf の値を覚え、操作の後も変わらないこと（「no write while the volume changes」）。
    - session の log に `ZWL VOLUME kept` が無いこと。
    - Log Out の後に `sound.volume` が操作の後の値になっていること（「kept in desktop.conf at Log Out」）。
    - 次の login で audiod に戻ること（今まで通り）。
  - `plan/ws100/tests/volume-p005.sh`: Settings の slider の後も desktop.conf が変わらないこと（「settings slider: desktop.conf not written」）。
  - `plan/ws100/tests/volume-bug153.sh` の 3 段目: 手で書いた desktop.conf の値に従わないこと、session の間の `kept` が 0 であること。

## ws131-p004 の T2-006 の `mute off: audiod unmuted (1) FAIL`

- Q1 の指示で一緒に調べた。T2 の証拠（`/home/awe/zedBSD-worktrees/t2/build/t2-006/volume-p004-retry/session-first.log`）では、zdesktop は mute off の `ZWL VOLUME set value=75 muted=0 via=mute final=1` を出して送り、`send errno` も出ていない。
- backend の audio（`audio-zedbsd.c`）は p004 で名前を変えただけで、送る中身は同じ。compositor は audio-compat を通らない。
- よって test の 1 回の読み（click の 0.8 s 後）が、遅い guest の audiod の答えより先だったと見ている。fstrim の I/O の停滞と重なった時間で、断定はしていない。
- `volume-p004.sh` の mute の確認は、audiod の mute を最長約 4 s 待ってから判定するようにした（`audiod_muted_wait`）。mute の行は `mute.log` に残す。

## 確認（host）

| 確認 | 結果 |
| --- | --- |
| zedBSD の build（`bin/wayland`・`bin/settings`） | exit 0、warning 0 |
| Linux の build（native の gcc） | exit 0、warning 0 |
| Settings の host 試験（`plan/ws089/tests/host-build.sh`・`host-wallpaper.sh`） | build、PASS |
| 3 つの試験の script | `sh -n` が通る |

## QEMU の試験（T2 に依頼、結果待ち）

volume の image で volume-p004・volume-p005（ws131-p004 の T2-006 の再試験を兼ねる）、volume-bug153。

## 結果（Q1、2026-10-03）

cleared。T2-009（QEMU、efc5ea7fc、build warning 0）: volume-p004 PASS（136 s）、volume-p005 PASS（83 s）、volume-bug153 PASS（70 s）。証拠 worktrees/t2/build/t2-009/。実機は未実施。
