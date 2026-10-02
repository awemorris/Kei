# q577 / P8 診断 checkpoint

2026-10-02 UTC。実行base: `41aac4fc76d0038f9024b0b966d8ae93eb4145fa`。
Phase は in-progress。BUG-125 は reproduced / tracking を保持する。

## 前提と環境

- q577-i01 の承認snapshotとphase.mdを照合。AGENTS.md、Guardrail、C全文、automation、agent protocol、WS099/guide/BUG-125、関連実sourceを読み込んだ。
- QEMU 10.0.11 / Python 3.13.5、zedBSD Venus。rendererはMAINの `build/ws035-sq-venus/install` を読取だけで使用。toolchain/NoctLangのbuild・install・変更無し。
- imageはMAINの `build/ws105-p011-criteria/hdd-image.img` からP8の `build/p8-q577/criteria.img` へ複写。SHA256 `992e83f498cda2a1d506bb6ad7975b3c4592069fcb6493eca707e0d84403ac37`。
- imageのsource provenanceはq538の記録にあるtarget `8fb181054405f072205f09c827aed005a243e490`。image内部の全build provenanceを独立に証明してはいない。現行HEADまでwindow関連toplevel.c/seat.c/input.c/shell.cは差分無し。popup-probeは旧 `userland/base/tests/popup-probe/main.c` と現行 `userland/tests/popup-probe/main.c` のSHA256がともに `2ade619345ff17bcd50a85428f816c999657d4ebb2ae70fd9de14804d6826658`。
- 既存binary hash: wayland `2cf785aea72e344ba1f957ba481328d87afcdb62d01f130ddf65fa7bc47a5732`、popup-probe `594eb6bf336b6ff9536c9146dc48a13feae884f368d38f15f98c41557b6cd67c`。現行build合格の代替とはしない。
- P8 runtime `build/p8-q577/runtime` は独立disk/QMP/VNC/serial/socketを持つ。SSH/gdbはguest.pyが空きloopback portを割当。MAIN runtimeは操作していない。
- guest鍵はP8の既存複写を使用、公開鍵一致を確認。鍵内容は証拠・commitに含めない。
- MAINのsync ownerはq576 finishedの記録、P8にsync cache無し。共有計画/syncはmain所有、GitHub公開保留。

## 起動・初回実行

`OUTPUT=build/p8-q577/boot BOOT_TIMEOUT=180 bash plan/tools/boot-test.sh build/p8-q577/criteria.img` PASS。
[保存したlogin PNG](evidence/boot-login.png)を目視しlogin promptを確認。framebufferのみの起動判定であり、graphical sessionの受け入れではない。console/serial logは読んでいない。

未変更の `GUEST_RUNTIME=build/p8-q577/runtime sh plan/ws035/tests/zdesktop-p076.sh build/p8-q577/baseline-1` はPASS。move 540,393、corner 550x400、left settled 890,393 / 200x400、wide settled 290,393 / 800x400と各期待画素が一致。単独1回の成功は修正・原因特定を証明しない。

## 既存FAILの分類

q538の元 `plan/history/ws105/q538/evidence/criteria/c9-p076/zdesktop.log` はcorner 550x400 settled後、leftに `ZWL RESIZE refused surface=8 reason=no-press` を記録。次のwide用pressはpopupを開いており、left resize開始を証明するlogが無い。q532もend geometry416x400（期待200）であり、pixel待ちだけの問題とは扱えない。

現行sourceではtoplevel.cの `press_held` がbuttons_down無しまたはserial不一致のrequestを拒否する。main.cはinputをまとめて読んでからclient requestを読む。固定時間のdragがclientからのresize request到着前にmotion/releaseを送る可能性は残るが、この時点では仮説。geometry失敗を描画待ちで隠す修正は行っていない。

次: 未変更baselineの再現、`--log-frames`によるevent順序の観測、必要なら新規resize startをcountで待つ診断との比較。製品sourceは読取のみ。20単独/5C9と実機は未実施、Phase clearance無し。

## 05:07 UTC checkpoint: request到着前のrelease

- 未変更baseline-2もPASS。`--log-frames`を足した元のp076全手順も1回PASS。各settledから次のcomposeはlog上1frameで、今回の成功試料からpixel待ちだけの修正を採用する根拠は無い。
- menu手順を省略しmap直後に同じstage7 move入力を送る診断で、**遅延注入より前に** `ZWL GLASS request move surface=8 refused=no-press` とexpected moved log MISSINGを再現。[試験出力](evidence/natural-move-no-press.txt)、[compositor全log](evidence/natural-move-no-press-frames.log)、[probe全log](evidence/natural-move-no-press-probe.log)。初期map440,293、button press local200,10、release local300,110。probeはmove要求とpongを出すがcompositorはheld無しとして拒否する。
- moved画素740,548は元の440,293 / 400x300の範囲にも含まれるためPASSした。expected moved logはMISSINGのままなので全試験のFAILは維持される。この1画素だけではmove成功を証明しない。
- 上記試料は短縮手順であり、全p076の自然再現率とは分ける。元のmenu操作で経過する時間が無い初期条件。後続cornerの操作は既にmove未達なので判定に使用しない。
- 遅延注入のprototypeはPID取得が空で、STOP/CONT信号は有効なtest compositorへ送られなかった。後続ジェスチャーが一部進んだ後でhost scriptを停止して証拠回収した。**注入による416幅の因果証明は得ていない**。その部分を再現証拠に転用しない。
- 同じ短縮初期条件で、button held中の新規move requestとresize-start count（0→1→2→3）を観測してからmotion/releaseを送る診断は1回PASS。move540,393、550/200/800幅と各期待settled geometry・画素を維持。[比較出力](evidence/geometry-handshake-1.txt)。このprototypeはtimeout時のabort/transport上限をまだfinal-reviewしていないため、共有p076へ適用していない。
- mainの05:04 UTC通知: 現行main `5ac9b753d` のfresh passthrough-demo build warning0、wayland hash `2cf785aea72e344ba1f957ba481328d87afcdb62d01f130ddf65fa7bc47a5732` が今回imageのbinaryと一致。対象compositorのcurrent build一致を支持するが、kernel/config/image全体の証明には使わない。

次: held中の新規requestを有限に待ち、timeout時はreleaseしてFAIL終了する診断に整える。追加注入なしでgeometry基準を保った比較を行う。mainの追加指示によりrequest handshakeはq577内のharness同期候補、製品sourceは読取だけ。単独20/C9 5はまだ未実施。
