<!-- awesome-plan project=zedbsd record=ws118-remote-log -->
# Latitude 5320 の遠隔の log の取り方（ws118-p001）

2026-10-02、P1。画面（内蔵 LCD）に頼らずに、5320 を USB から起動して host（centris）から SSH で log を取る手順。
ここに書いた確認はすべて **QEMU の証拠**（[phase001](phase001/phase.md) の結果）で、5320 の実機では未実施（実機は p002 でユーザーと一緒に）。

## 1. image（3 種）

`plan/ws118/tests/build-remote-log-image.sh VARIANT BUILD [ADDRESS/PREFIX GATEWAY [DNS]]` で作る（`plan/ws075/demo/build-demo-image.sh` を呼ぶだけ）。
利用者に渡す image は `REMOTE_LOG_LEAN` を付けずに build する（付けると clang・libcxx・remacs が抜ける。エージェントの worktree の確認用）。

| 種 | 中身 | 画面 | 使う場面 |
| --- | --- | --- | --- |
| A | デモの image そのもの（i915、graphical boot、`display=edp`、kei の自動の login） | splash → greeter/desktop | 失敗を本番の形で再現する |
| B | i915 あり、graphical boot 無し（kernel の message を画面に）、boot の行 `display=edp login=graphical`（`login=graphical` は一つだけ）、自動の login 無し | kernel の message | 画面が少しでも映るなら、どこまで進んだかを見る |
| C | i915 無し（firmware の framebuffer だけ）、kernel の message を画面に、自動の login 無し | kernel の message | i915 が起動を止めるときの退路（SSH に届く） |

どれも起動時に sshd が立ち、root に `plan/tmp/guest/id_ed25519` の鍵で入れる（公開鍵は image の `/root/.ssh/authorized_keys`）。
鍵の組が無ければ `plan/tools/guest/guest.sh keys` で作る（build の script が無いときに呼ぶ）。passthrough の VBT は入れない。
root の crontab（`plan/ws118/tests/root-crontab`）が 1 分ごとに kernel の message を `/var/log/dmesg.cron` に写す（4 の退路のため）。

## 2. network

- 既定（DHCP）: image の既定の `/etc/net.conf` のままで、networkd が USB の LAN（`ue0`、RTL8156 の CDC NCM など）に DHCP をかける。
  QEMU の usb-net で、default route と resolver が `ue0` の lease で入ることを確かめた。
  host から address を知る方法: (a) 下の固定 IP の変種、(b) 利用者の LAN の router の DHCP の割当ての表で、5320 の USB の LAN の MAC の行を見る。
- 固定 IP の変種: `build-remote-log-image.sh C build/rl-c-fixed 10.0.10.50/24 10.0.10.1` のように address・gateway（・DNS）を付けると、
  `ue0` を固定 IP にした `/etc/net.conf` が image に入る。後から変えるときは、USB の root の UFS の `/etc/net.conf` を書き換える
  （書き換えは zedBSD か UFS を書ける環境で。ESP の `zedbsd.cfg` では変えられない）。
- 無線（Archer T3U、RTL8822BU）を使うとき: 有線が無いときだけ。5320 で root として `net wifi set-key 'SSID' 'PASSPHRASE' auto` を打つ
  （鍵は root の store `/etc/wifi.conf` に入り、起動時の `net startup` が自動で接続する）。画面に頼れないので、一度有線か C の画面の console で
  入れておく。鍵は plan・git・log に書かない。

## 3. ユーザーの操作

1. image を USB に書く（利用者の方法。例: `sudo dd if=hdd-image.img of=/dev/sdX bs=4M conv=fsync status=progress`、Windows なら Rufus／balenaEtcher）。
2. 5320 に USB の LAN をつなぎ、centris と同じ LAN（または router）に挿す。
3. 5320 の電源を入れ、F12 の一回の起動 menu から USB（UEFI）を選ぶ。
4. 2〜3 分待つ（初回は OpenSSH が host key を作るので遅い）。画面は見なくてよい（見えたら写真を撮ってもらう）。

## 4. エージェントの操作

1. address を知る（固定 IP か、router の割当て）。
2. `plan/ws118/tests/collect-5320.sh ADDRESS OUTDIR` で log を集める（`dmesg`、`lspci -Dkv`、`lsusb -tv`、`sysctl -a`（`hw.gpu.attaching`・
   `kern.boot.*` を含む）、`ifconfig -a`、`net show`、`route -n show`、`/etc/resolv.conf`、`net wifi list`、`/dev` の一覧、`mount`、
   `/var/log/messages`、`/var/log/sessiond.log`・`greeter.log`、`/run/user/*/session.log`、`service list`、`ps ax`）。各項目は `OUTDIR/NAME.txt`、
   終了の状態は `OUTDIR/index.txt`。
   - i915 の診断の node は zedBSD に無い。i915 の情報は `dmesg` と `hw.gpu.attaching` だけ。p002 で足りなければ i915 の担当に node を依頼する。
   - QEMU（i915 無し）では sessiond は `no-display` で session を作らないので、session の log の項目は「無い」になる（5320 では在るはず）。
3. hang して SSH に届かないとき（退路）:
   - まず C を試す（i915 無し）。C でも届かなければ、電源を切り、USB を centris に挿して root の UFS を読む:
     `python3 plan/ws031/tests/ufs-cat.py /dev/sdX /var/log/messages /var/log/dmesg.cron`（device の代わりに `dd` で吸い出した image でもよい）。
   - `/var/log/messages` に入るのは userland の message だけ（syslogd は kernel の message を `/run/dmesg.boot`（tmpfs）にしか置かない）。
     kernel の message は cron の写し `/var/log/dmesg.cron`（最大 1 分前まで）にだけ残る。cron が起動する前（kernel の i915 の attach の途中など）の
     hang では disk に kernel の message は残らない。そのときは B の画面の写真が頼り。
   - 電源を急に切ると UFS の journal に残った最後の書込みは読めないことがある（`ufs-cat.py` の注意）。
