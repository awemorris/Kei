<!-- awesome-plan project=zedbsd record=ws089-p020 -->
# ws089-p020: Wallpaper の頁の縮小表示を別の thread で読む（BUG-152）

Status: in-progress（実装済み・T1 の試験待ち）
Disposition: normal
Parent: [WS089](../ws.md)
Queue: q638（P1 generation11、2026-10-03。承認: user「次のセッションはP1とT1を起動、実機がなくても修正できるバグをP1で修正、T1で順次テスト、のパイプラインを実行してください。」「実行してください。」）
Bug: [BUG-152](../../bugs/BUG-152.md)

## 範囲

S1 の実機（5330、USB 起動）で Wallpaper の頁を開くと約 10 秒止まる。原因: `se_wallpaper_draw` → `se_look_scan` が描画の thread で
7 枚の PPM（各 6MB）の縮小を同期で読んでいた（1 枚あたり 450 回の pread）。user「画像読み込みはマルチスレッドにしないとだめだね。要修正。」
PPM→PNG の切替は F-071 で別（ここではしない）。

## 実装

- `look.c`: `se_look_scan` は一覧（opendir・名前）だけを作り、`look_load_start` が読み込みの thread を起こす。thread は path の複写だけを
  読み、各縮小を lock の下で `done[i]` と共に渡す。描画の thread は `se_look_poll` の頭の `look_load_take` で受け取り、tile を埋めて
  frame を要求する。全部受けたら join（log `LOOK pictures ready count=N`）。窓を閉じるときは `look_load_stop` が stopping を立てて join し、
  受け取られていない縮小を解放する。thread を起こせない時は縮小なし（描いた代わりの絵）で頁を出す（止めない）。
- `se_look_wait`（新、40ms）を `main_timeout` に足し、読み込みの間は 40ms ごとに受け取る。
- `page-look.c`: 読み込み中の tile は灰色の代わり（`pending`）。読めなかった物は従来の Kei の mark。
- `settings.h`: `struct se_look_loader`、`se_wallpaper.pending`、`<pthread.h>`。Makefile.freebsd・linux に `-pthread`（zedBSD の libc は pthread を含む）。
- 試験: `host-render.c` の `draw=` は読み込みの完了を待ってから描く。guest の `settings-p004.sh`・`settings-p009.sh` は
  `LOOK pictures ready count=6` を待ってから撮る。新しい host 試験 `tests/host-wallpaper.sh`。

## 検証

- build: `make -j8 ZEDBSD_CONFIG=plan/ws089/tests/config-amd64-settings.mk BUILD=build/p1-q638 build/p1-q638/bin/settings` → exit 0、warning 0。
- host: `sh plan/ws089/tests/host-wallpaper.sh` → `host-wallpaper: PASS`（4 枚: 3 枚 error=0、壊れた 1 枚 error=22、`loader started` が
  最初の `LOOK picture path=` より前、`pictures ready count=4`）。画面 `build/ws089-host/wallpaper/page.png` で縮小が埋まっている。
- `git diff --check` PASS。
- QEMU（T1 に依頼）: `settings-p004.sh`・`settings-p009.sh`。未実施（結果待ち）。
- 実機（5330 の USB）で 10 秒の停止が消えたかは未実施（次の安定版の実機試験）。
