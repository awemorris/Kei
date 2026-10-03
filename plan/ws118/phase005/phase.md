<!-- awesome-plan project=zedbsd record=ws118-p005 -->
# ws118-p005: i915 を起動の後に手で初期化して 5320 の LCD を debug する

Status: uncleared（q634-i01、P4、2026-10-03 host 再起動のためのラップアップで中断。後で再開）
Disposition: normal
Parent: [WS118](../ws.md)
Focused goal: fg019（ベータ1）
Queue: q634
依存: p001 の image の道具（未実施の残りは q634 で仕上げる）。実機の起動・USB への書き込み・目視はユーザー
目安: 4〜6h ＋ ユーザーの立会い

## 由来

2026-10-03 user:「実機テストを加速したいので、11th gen Core i5のマシンで、i915のLCDが点灯しない問題を解決します。P4を立ててこれを割り当てます。sshが有効なディスクイメージを作成し、起動時にはi915を初期化しないことにします。その上で、SSHで接続できたあとで、i915を初期化し、デバッグメッセージを得ることで、LCDの初期化をデバッグします。これが完了すれば、利用できるi915実機が2台になり、1台が使用中でも私が実機確認できます。」

## 範囲

1. **変種 D の image**: p001 の A・B・C に並べて、i915 を kernel に入れたまま起動の時には bring-up しない image を作る（sshd、USB の LAN の DHCP、root の鍵、自動の login 無し、firmware の framebuffer）。
2. **起動の後の手での初期化**: 起動の option（例: `i915=defer`）で i915 の deferred start（`src/drivers/gpu/i915/i915.c` の attach が登録する遅延の bring-up）を止め、SSH の中から root が明示に始める手段を足す（sysctl・device の node・command のどれかを P4 が設計して Q1 に返す）。既定の起動の挙動は変えない。
3. **debug の message**: LCD の初期化の経路（VBT/OpRegion の panel の情報、eDP の AUX・DPCD・link training、panel の power sequence・backlight、PLL・DDI・pipe・plane、hotplug）の詳しい log を、option（例: `i915.debug=`）で出し、`dmesg` と SSH で取れるようにする。
4. **QEMU での確認**: D が起動して SSH で入れ、手での初期化の経路が i915 の無い QEMU でも安全に失敗する（panic しない）。5330 の passthrough は使わない（iGPU を使う i915 の作業の規則に従うなら Q1 に確かめる）。
5. **実機（ユーザーと）**: ユーザーが 5320 で D を起動 → SSH で入り → 手で初期化 → log を取り、LCD が点かない原因を分類して直す（修正の規模が大きければ p003 に分ける）。5330 の表示を壊さないことを確かめる（5330 の確認はユーザーと時期を合わせる）。

## 受け入れ

- D の build（warning 0）、boot-test、QEMU の SSH の login と手での初期化の安全な失敗。
- 実機: 5320 で SSH から i915 を初期化し、LCD の初期化の log を取れる（実機の証拠、ユーザーの立会い）。原因の分類と、修正で 5320 の LCD が点く、または残りの原因と次の Phase。
- 既定（option 無し）の起動で i915 の挙動が変わらない（5330 で回帰しない）。

## 規則の注意

- HAL の API（`include/hal/hal.h`）は変えない。変えるなら差分を plan に置いて Q1 経由でユーザーの承認。
- 実機の操作の前に、Q1 経由でユーザーに時期を確かめる。QEMU の証拠と実機の証拠を分ける。

## q634-i01 の途中の結果（P4 generation1、2026-10-03、base main `5a3aca28c`、ラップアップで中断）

設計（Q1 承認 2026-10-03。kernel の path: boot.h・boot.c・uapi/sysctl.h（追加だけ）・kern/sysctl・klog.c・docs、syslogd の修正も Q1 に通知済み）:

- 起動 option `i915.start=auto|manual`（既定 auto）と `i915.debug=off|display`（既定 off）。他の語・空・重複は parse error
  （`src/kern/boot.c`、`include/kern/boot.h`）。i915 は readiness の時に読む（boot の parameter は PCI attach の後に parse されるため）。
- manual: attach した device を pending のまま保留し、`hw.gpu.attaching` から外す（login=graphical はすぐ console に戻る）。
  sysctl の新しい leaf `hw.gpu.start`（uint64、`include/uapi/sysctl.h` の `HW_GPU_START 5`）: 読み＝保留中の数、root の
  `sysctl hw.gpu.start=1`＝保留を解いて start worker を起こし、attaching に数える。保留が無い・i915 が無い（ops 未登録）・
  Intel GPU が無いときは ENODEV で何も変えない。kern→driver は `kern_gpu_start_ops_set()`（`include/kern/sysctl.h`、
  `src/kern/sysctl.c`）、i915 は driver の登録で ops を入れる（`src/drivers/gpu/i915/device.c`・`i915.c`・`device.h`）。
  userland の sysctl の command は汎用の数値の書込みでそのまま使える。
- `i915.debug=display`: Linux の本文の debug の message（`drv_i915_lcd_kernel_debug`、今まで捨てていた）を `i915: LCD-B debug:` で出し、
  resident run の enable commit の return で run log を全部出し、run の終わりの run log・observer を成否に関わらず出す
  （`display/diagnostics.c`・`diagnostics.h`・`modeset.c`）。既定の i915 の log（N0・P3〜P7・edp・LCD-A など）は元から多い。
- amd64 の kernel の log の ring を 32 KiB → 512 KiB（`src/kern/klog.c`、他の arch は 32 KiB）。読む側: dmesg（上限 1 MiB）と
  sysctl の syscall（上限 1 MiB）は変更不要。syslogd の `save_boot_log` は 64 KiB の固定 buffer で 64 KiB を超えると
  /run/dmesg.boot を書けなくなるので、size を問うて malloc する形に直した（`userland/base/syslogd/main.c`）。
- docs: `docs/reference/kernel-boot-parameters.md` 7c 節。
- 道具: `plan/ws118/tests/build-remote-log-image.sh D BUILD`（boot の行 `display=edp login=graphical i915.start=manual
  i915.debug=display`、`ZEDBSD_GRAPHICAL_BOOT=n`、自動 login 無し）、`plan/ws118/tests/manual-start.sh HOST OUTDIR [GREETER_SECONDS]`
  （`sysctl hw.gpu.start=1` → node の公開か失敗まで 2 秒ごとに dmesg を保存 → `service start greeter` で panel の modeset →
  dmesg の保存 → collect-5320.sh）、host 試験 `plan/ws118/tests/host-boot-i915-test.sh`。手順は [remote-log.md](../remote-log.md) 5 節。

確認（2026-10-03、P4 の worktree、QEMU と実機は未実施）:

- host 試験: `plan/ws118/tests/host-boot-i915-test.sh` 19 checks 0 failures。既存の `plan/ws075/tests/hdmi/host-output-test.sh`
  80 checks 0 failures（boot.c の他の parameter が変わらない）。
- D の build: `REMOTE_LOG_LEAN=y plan/ws118/tests/build-remote-log-image.sh D build/p4-rl-d` rc=0、続けて最終の source で
  incremental に再 build して rc=0。zedBSD の source の warning・error は 0（log の grep、外部 package の configure の
  warning は除く）。cfg は上の boot の行のとおり（login=graphical は 1 行）。image: worktree の `build/p4-rl-d/hdd-image.img`
  （LEAN のため clang・libcxx・remacs を含まない。LCD の debug には不要）。
- C の build は失敗（phase001 の記録、運用の誤り、source の不具合ではない）。

未実施（再開点）:

1. C の build（D と並行させない）。A の build（既定の起動の確認用）。
2. QEMU（`plan/ws118/tests/qemu-ssh-check.sh build/p4-rl-d/hdd-image.img PORT OUTDIR` を KEEP で、または同じ形）で D に SSH で
   入る。`SSH_PORT=PORT plan/ws118/tests/manual-start.sh 127.0.0.1 OUTDIR 20` で、`hw.gpu.start` が 0、`hw.gpu.start=1` が ENODEV で
   失敗し panic しないこと、dmesg に `i915.start=manual: 0 device(s) held` と `hw.gpu.start: no device is held` が出ること。
   C でも `sysctl hw.gpu.start=1` が ENODEV（ops 無し）。
3. `plan/tools/boot-test.sh` で D と A（A は既定の自動の start が変わらないこと）。D の画面に console の login prompt が出ること
   （user が root/root で login して `ifconfig ue0` を打つ手順の前提、2026-10-03 Q1 の確認依頼）。出ないなら /etc/issue 等の代案を Q1 へ。
4. 確認が済んだら WIP commit を Q1 に merge 依頼し、user に渡す image の path・書き込み・SSH の手順を Q1 に送る（user に渡す image は
   main の merge 後に作るか worktree の物を渡すかを Q1 と決める）。
5. 実機（user）: D を起動 → user が console で root で login し `ifconfig ue0` の address を Q1 に → P4 が manual-start.sh で log を回収・解析。

解析の準備のメモ（未検証の候補、実機の log で確かめる）: `display/modeset-internal.h` の `IS_TIGERLAKE_UY(i915)` は常に 0（Linux は TGL の
UY の device ID で真、HBR2 の DP/eDP の buf trans の表が変わる）。N0 の VT-d の PMR（firmware の DMA protection）は STOP の条件で、
5320 の BIOS 設定次第で表示が absent になる（既定の log の `i915: N0 decision` で分かる）。

## main への統合（Q1、2026-10-03）

ユーザーの指示「サブエージェントの成果で未統合のものがあれば、統合してmainにコミットをお願いします。」で、agent/p4 の 59a94c6aa（source）と WS118 の記録・試験を main に取り込んだ（branch 全体の merge は古い共有の記録を持ち込むので、source の差分と plan/ws118 だけを当てた）。
確認: main の `make -j16 vmunix` は warning・error 0、kernel include check と amd64 vmunix check PASS、`plan/tools/../ws118/tests/host-boot-i915-test.sh` 19 checks 0 failures。boot-test・QEMU の SSH・既定の起動の不変（A）は未実施（上の再開点 1〜3 のまま）。
