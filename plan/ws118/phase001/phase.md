<!-- awesome-plan project=zedbsd record=ws118-p001 -->
# ws118-p001: 5320 の遠隔の実機 log 用 image（sshd）と手順

Status: planned
Disposition: normal
Parent: [WS118](../ws.md)
Focused goal: fg019（ベータ1）
Queue: none（未承認）
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
