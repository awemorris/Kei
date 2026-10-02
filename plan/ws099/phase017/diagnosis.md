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
