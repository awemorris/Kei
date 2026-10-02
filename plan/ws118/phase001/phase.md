<!-- awesome-plan project=zedbsd record=ws118-p001 -->
# ws118-p001: 5320 の遠隔の実機 log 用 image（sshd）と手順

Status: uncleared（q601-i02、P1、2026-10-02 Q1 の割り込み（q599-i04）で中断。後で再開）
Disposition: normal
Parent: [WS118](../ws.md)
Focused goal: fg019（ベータ1）
Queue: q601 / q601-i01（P1、中断）
目安: 2〜3h（build と QEMU だけ、実機は使わない）

## 範囲

2026-10-02 user「5320ではLCDの制御がうまくいっていないので、SSHDを起動してリモート実機でログを取れるようなイメージを作成して、ユーザと一緒に進めましょう。」

1. build の script `plan/ws118/tests/build-remote-log-image.sh VARIANT BUILD` を作る。`plan/ws075/demo/build-demo-image.sh` を呼ぶだけにし（その file は変えない）、次の 3 種を作る:
   - **A** 通常（i915 あり、graphical boot、`display=edp`）: 本番に近い形で失敗を再現する。
   - **B** i915 あり・kernel の message を画面に（`ZEDBSD_GRAPHICAL_BOOT=n` と `ZEDBSD_BOOT_EXTRA_LINES=display=edp login=graphical` の組、`plan/tools/hw5330/README.md` 3.4 の注意: `login=graphical` を重ねない）。
   - **C** i915 無し（`CONFIG_DRIVER_PCI_I915=n` の config の断片、firmware の framebuffer だけ）: i915 が起動を止める場合でも SSH に届く退路。
   どれも sshd が起動し、root に `plan/tmp/guest/id_ed25519.pub`（無ければ作る手順）の鍵で入れる。kei の自動の login は A だけ。`passthrough` の語（`I915_TEST_VBT`）は付けない。
2. network: USB の LAN（RTL8156 の NCM、`ue0`）を `networking` の起動で DHCP にする設定を image に入れる（`/etc/net.conf` に `ue0` の dhcp、既定の net.conf で足りるなら入れない、を確かめる）。
   DHCP の address を host が知る方法として、(a) 固定 IP の変種（`zedbsd.cfg` か net.conf の書き換えで後から変えられる形）、(b) 利用者の LAN の DHCP の割当てを router で見る、の両方を手順に書く。
   無線（Archer T3U）を使う場合の `/etc/wifi.conf` を利用者が USB の上で書く手順も書く（鍵を plan・git に入れない）。
3. log の回収の道具 `plan/ws118/tests/collect-5320.sh HOST OUTDIR`: SSH で `dmesg`、`lspci`（`-v` 相当があればそれ）、`lsusb`、`sysctl` の hw 系、`net show`、`/var/log/messages`、
   `/run/user/*/session.log`、i915 の状態を読む手段（あれば debug の node。無ければ「無い」と記録し、p002 で要るなら i915 の担当に依頼する）を集める。
4. hang したときの退路: 電源を切った USB を centris に挿し、UFS の `/var/log/messages` を `plan/ws031/tests/ufs-cat.py` で読む手順（kernel の message が syslogd で disk に入るかを QEMU で確かめる）。
5. 手順書 `plan/ws118/remote-log.md`: ユーザーの操作（USB への書き込みは利用者の方法、BIOS の boot の選択、LAN の接続）と、エージェントの操作の順。

## 受け入れ

- A・B・C の image を build し（warning 0）、A と C を `plan/tools/boot-test.sh` で login prompt まで（PNG をユーザーに見せる）。
- QEMU で USB 起動・usb-net・host から guest への SSH（`plan/ws033/tests/ssh-host-to-guest.sh` の形、WS033 の file は変えずに呼ぶか写す）で、A と C に鍵で入り、
  `collect-5320.sh` が全部の項目を取れる（取れない項目は理由を記録）。4 の disk の log の確認。
- QEMU の証拠であって 5320 の証拠ではないと書く。実機は p002。

## 所有 path

`plan/ws118/`（`tests/`・`remote-log.md`・config の断片）、自分の worktree の `build/`。source は変えない（source の変更が要るなら main に返す）。

## 依存

なし。ユーザーへの確認（p002 の前までに答えがあればよく、このPhaseを止めない）: 5320 の LAN の手段（RTL8156 の USB の LAN があるか、Archer を使うか）、
5320 をつなぐ LAN（centris と同じ 10.0.10.x か）と固定 IP の希望。

## 未決の判断

上の確認。i915 の診断の node が無い場合に作るか（i915 の source、WS118 p003 か i915 の WS で）。

## q601-i01 の途中の結果（P1、2026-10-02、base `66d27ed7c`、中断）

- 作った: `plan/ws118/tests/config-remote-log.mk`（デモの config＋`REMOTE_LOG_NO_AUTOLOGIN`・`REMOTE_LOG_NETCONF`・`REMOTE_LOG_LEAN` の切替）、
  `config-remote-log-c.mk`（i915=n）、`empty-autologin`（B・C の自動 login を消す空の `/etc/keiland/autologin`）、`build-remote-log-image.sh A|B|C BUILD
  [ADDRESS/PREFIX GATEWAY [DNS]]`（`build-demo-image.sh` を呼ぶだけ。固定 IP は ue0 の static と default route・DNS の net.conf を生成）。
- build（`REMOTE_LOG_LEAN=y`、clang・libcxx・remacs を除く。利用者に渡す image は除かない）: A・B・C とも rc=0、zedBSD の source の warning 0。boot の行:
  - A: `logo=logo.ppm login=graphical kmsg=quiet display=edp`
  - B: `video=640x480 display=edp login=graphical`（login=graphical は一つ）
  - C: `video=640x480 display=edp`（graphical boot 無し、kernel の message を画面に）
- 未実施: boot-test（A・C）、QEMU の USB 起動＋usb-net＋SSH での鍵の login、`collect-5320.sh`、disk の log の退路の確認、固定 IP の変種の build、`remote-log.md`。
  B・C の自動 login の無効化（空の autologin の上書き）が image に入ったかの確認も未実施。

## q601-i02 の途中の結果（P1 generation2、2026-10-02、base main `55ff880b4`、中断）

- A・B・C を main `55ff880b4` で build し直した（`REMOTE_LOG_LEAN=y`、rc=0）。B・C の `/etc/keiland/autologin` は 0 byte（自動の login 無し）、
  A は 4 byte（kei）。C の vmunix に i915 の symbol は 0。boot の行は q601-i01 の記録どおり。
- 作った: `tests/collect-5320.sh HOST OUTDIR`（SSH で 17 項目、`SSH_PORT` で port）、`tests/qemu-ssh-check.sh IMAGE PORT OUTDIR`（UEFI、USB の stick と
  usb-net を一つの xHCI に、hostfwd、sshd を待って collect、`logger` の印と `sync`、`KEEP=1` で stick を残す）、`tests/root-crontab`（root の
  crontab: 1 分ごとに `dmesg` を `/var/log/dmesg.cron` へ）とそれを入れる `config-remote-log.mk` の行、手順書 [remote-log.md](../remote-log.md)。
- QEMU（A、crontab を足す前の build）: USB 起動、usb-net の DHCP（`ue0` 10.0.2.15、default 10.0.2.2 ue0、resolver ue0）、sshd が 113 秒で鍵の login を受けた。
  collect の 17 項目は全部取れた（sessiond の log は `no-display` の 1 行、`/var/log/greeter.log`・`/run/user/*/session.log` は無い: QEMU の std VGA で
  greeter が立たないため）。i915 の診断の node は無い（`dmesg` と `hw.gpu.attaching` だけ）。
- 退路の確認（QEMU、A）: 電源断の後の stick の `/var/log/messages` を `ufs-cat.py` で読めた（`logger` の印あり）。**kernel の message は入っていない**
  （syslogd は kernel の buffer を `/run/dmesg.boot`（tmpfs）にだけ書く）。それで root の crontab を足した（確認は未実施）。
- 未実施（再開点）: crontab 入りの A と C の `qemu-ssh-check.sh`（C も）、`/var/log/dmesg.cron` が stick に残ることの確認、A と C の `boot-test.sh`、
  固定 IP の変種の build と QEMU の確認（例: `C build/p1-rl-cf 10.0.2.50/24 10.0.2.2`）、remote-log.md の見直し。
- 追記（ラップアップ、2026-10-02）: `qemu-ssh-check.sh` に、collect の後に root の crontab の `/var/log/dmesg.cron` を最大 90 秒待つ段を足した（未実行）。
  crontab 入りの A・C の SSH の確認はラップアップの指示で途中で止めた（結果無し）。再開点は上の「未実施」のとおり。
