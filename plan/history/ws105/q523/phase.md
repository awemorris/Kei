<!-- awesome-plan project=zedbsd record=ws105-p001 -->

# ws105-p001: Linux の試験の guest（QEMU の Debian 13）と操作の道具

Status: cleared
Disposition: normal
Parent: [WS105](../ws.md)
Queue: q523 / q523-i01
依存: なし
実行者: phase-runner（high）か phase-runner-mid。道具は `plan/tools/keiland-linux/`（main の範囲。subagent は自分の worktree で作り、main が merge して Tools 節に登録する）

## 判断（2026-10-01）

ユーザーは Linux guest の起動を SSH の疎通と QMP の `screendump` の PNG で確認する方法（[WS105 D25](../ws.md)）を許可した。接続はホストの `127.0.0.1:2225` から QEMU の host forwarding を経て、この Phase で作る Debian 13 guest の port 22 に届く。外部の SSH server は使わない。AGENTS.md と Guardrail に WS105 限定の例外を記録した。Queue は未設定。

## 目的

Keiland の Linux の compositor・app・gdm・WiFi・音を試す場所を作る（決定 D23・D25、[design.md](../design.md) §7）。host（この開発機）の画面と入力は使わない。
この Phase は Keiland の code を書かない。**手本は [survey/build-guest.sh](../survey/build-guest.sh)**（2026-10-01 に通った物。減らした package の組で 60 秒、KVM で起動し 9 秒で SSH）。

## 作る物（`plan/tools/keiland-linux/`）

| file | 役割 |
| --- | --- |
| `README.md` | この directory の道具の一覧と使い方（下の command をそのまま載せる） |
| `build-guest.sh [OUT] [VARIANT]` | Debian 13 の guest の disk image を作る（design §7.2 の「image の作り方」） |
| `guest.sh COMMAND ...` | guest の起動・停止・SSH・file の転送・screenshot・入力の注入（design §7.2 の「起動」「QMP」） |
| `install-guest.sh [STAGE]` | `DESTDIR` の tree（`build/keiland-linux/stage/opt/keiland`）を guest の `/opt/keiland` に写す |
| `png-probe.py PNG X Y [X Y ...]` | PNG の画素の色を出す（判定に使う） |

### `build-guest.sh [OUT] [VARIANT]`

- `OUT` の既定は `$PWD/build/keiland-linux/guest`（**絶対 path にする**: mmdebstrap の hook の `upload`・`download` が絶対 path を要る）。`VARIANT` は `base`（既定）か `gdm`
  （p009 で使う。`OUT` の既定は `$PWD/build/keiland-linux/guest-gdm`）。
- 作る物: `$OUT/guest.img`（raw、ext4、8 GiB）、`$OUT/vmlinuz`・`$OUT/initrd.img`、`$OUT/id_ed25519`（試験用の SSH の key、無ければ `ssh-keygen -q -t ed25519 -N '' -f`）、
  `$OUT/packages.txt`（入った package の版、`dpkg -l` を hook で取る）。
- 手順は survey/build-guest.sh のとおりで、design §7.2 との差を全て入れる: package に `dbus`・`libpam-systemd`（必須）、kei の group に `kvm`（lavapipe の `/dev/udmabuf`）、
  `systemctl disable wpa_supplicant.service`、`LC_ALL=C.UTF-8`、`PATH` に `/usr/sbin:/sbin`、`mkfs.ext4 -d rootfs.tar`、`gdm` の variant では `gdm3` と
  `/etc/gdm3/daemon.conf` の `[daemon]` に `AutomaticLoginEnable=true`・`AutomaticLogin=kei`。
- **sudo は使わない**（`--mode=unshare`）。
- 冪等: `$OUT/guest.img` があれば何もしない。`--force` のときだけ作り直す。**`--force` を既定の `OUT`（共有の image）に使うのは main だけ**（他の agent の試験が使っている）。
  agent が自分の image を作るときは `OUT` を自分の directory（例 `$PWD/build/ws105-p011/guest`）にする。
- 終わりに `rootfs.tar` を消す（大きい。image に入っている）。

### `guest.sh COMMAND ...`

環境変数（全て既定あり）: `GUEST_DIR`（`$PWD/build/keiland-linux/guest`）、`GUEST_IMAGE`（`$GUEST_DIR/guest.img`）、`GUEST_RUN`（`$PWD/build/keiland-linux/run`。pid・QMP の socket・
overlay・log）、`SSH_PORT`（2225）。**`GUEST_RUN` と `SSH_PORT` を変えれば別の guest が同時に走る。** 全て絶対 path にして使う。

| COMMAND | 動き |
| --- | --- |
| `start` | `mkdir -p $GUEST_RUN`、`qemu-img create -f qcow2 -b $GUEST_IMAGE -F raw $GUEST_RUN/overlay.qcow2`（毎回作り直す）、design §7.2 の QEMU の command を `-daemonize` で起動、SSH が通るまで待つ（上限 180 秒、1 秒ごとに `ssh ... true`）。走っていれば（pidfile の process が生きていれば）何もしない。PASS で `guest: ready` |
| `stop` | SSH で `poweroff`、30 秒で消えなければ QMP の `quit`、それでも残れば pid に `kill`。overlay を消す |
| `ssh CMD...` | `ssh -i $GUEST_DIR/id_ed25519 -p $SSH_PORT -o StrictHostKeyChecking=no -o UserKnownHostsFile=/dev/null -o LogLevel=ERROR root@127.0.0.1 CMD...`。`SSH_USER=kei` で利用者 |
| `put SRC DST`・`get SRC DST` | `scp -P $SSH_PORT`（同じ option） |
| `screenshot PNG` | `python3 plan/tools/qmp.py $GUEST_RUN/qmp.sock screendump '{"filename":"<PNG の絶対 path>","format":"png"}'` |
| `key KEY...` | 名前の列を全て押し、逆の順に離す（`key ctrl alt f2` など。QKeyCode の名前、design §7.2。super は `meta_l`）。event は全て `"device":"video0","head":0` |
| `type TEXT` | 英小文字・数字・空白・`-`・`.`・`/` を key の列にして 1 文字ずつ送る（`shift` が要る文字は対象の外） |
| `move X Y`・`click X Y [left|right|middle]` | 画素の座標を `x*32767/(幅-1)` に写して `abs` の event（幅・高さは `screenshot` の PNG を一度撮って `png-probe.py --size` で得る。既定 1280×800） |
| `status` | 走っているか、SSH が通るか |

### `install-guest.sh [STAGE]`

`STAGE` の既定は `build/keiland-linux/stage`。
`tar -C $STAGE/opt -cf - keiland | guest.sh ssh 'rm -rf /opt/keiland && tar -C /opt -xf -'`。
`$STAGE/usr/share/wayland-sessions/keiland.desktop` があれば `guest.sh put` で `/usr/share/wayland-sessions/` に写す（p009）。

### `png-probe.py PNG X Y [X Y ...]`・`png-probe.py --size PNG`

Python の標準 library（`zlib`・`struct`）だけで PNG（8 bit の RGB・RGBA、interlace 無し、filter 0〜4）を読み、点ごとに `X Y #rrggbb` を出す。`--size` は `WIDTH HEIGHT`。

## 手順（host、repo の root で）

```
sh plan/tools/keiland-linux/build-guest.sh                                    # build/keiland-linux/guest/guest.img（初回は数分）
sh plan/tools/keiland-linux/guest.sh start
sh plan/tools/keiland-linux/guest.sh ssh 'uname -r; ls /dev/dri/card0 /dev/snd/controlC0; ls /dev/input/'
sh plan/tools/keiland-linux/guest.sh ssh 'env -u WAYLAND_DISPLAY vulkaninfo --summary 2>&1 | grep -E "deviceName|driverName"'
sh plan/tools/keiland-linux/guest.sh ssh 'modprobe mac80211_hwsim radios=2 && iw dev | grep Interface'
sh plan/tools/keiland-linux/guest.sh ssh 'amixer -c 0 scontrols'
sh plan/tools/keiland-linux/guest.sh ssh 'id kei'
sh plan/tools/keiland-linux/guest.sh screenshot $PWD/build/keiland-linux/p001-console.png
python3 plan/tools/keiland-linux/png-probe.py --size build/keiland-linux/p001-console.png
sh plan/tools/keiland-linux/guest.sh key a
sh plan/tools/keiland-linux/guest.sh click 100 100
GUEST_RUN=$PWD/build/keiland-linux/run2 SSH_PORT=2226 sh plan/tools/keiland-linux/guest.sh start      # 2 つ目を同時に
GUEST_RUN=$PWD/build/keiland-linux/run2 SSH_PORT=2226 sh plan/tools/keiland-linux/guest.sh stop
sh plan/tools/keiland-linux/guest.sh stop
```

## 完了の条件

1. `build-guest.sh` が通り、`guest.sh start` が `guest: ready` を出す（180 秒以内）。
2. guest の中で: `uname -r` が `6.12`、`/dev/dri/card0` と `/dev/snd/controlC0` と `/dev/input/event*` がある、`vulkaninfo --summary` に `llvmpipe`、`iw dev` に 2 つの interface、
   `amixer -c 0 scontrols` に `'Master'`、`id kei` に `kvm`・`video`・`input`・`audio`・`render`・`netdev`。
3. screenshot の PNG に console の login の画面（文字）が出る（PNG をユーザーに見せる）。`png-probe.py --size` が `1280 800`。
4. `guest.sh key`・`click` が QMP の error なしで返る。
5. 2 つ目の guest が同時に起動・停止できる。
6. `stop` の後、`build/keiland-linux/run/overlay.qcow2` が無く、`guest.img` の時刻が変わっていない（共有の image を書いていない）。

## 記録

- 作った image の大きさ、mmdebstrap の時間、`packages.txt` の主な版（`mesa-vulkan-drivers`・`libvulkan1`・`weston`・`linux-image-amd64`）。
- master の Tools 節への登録（main）: 「Linux の試験の guest と道具（WS105）: `plan/tools/keiland-linux/`」。

## 結果

Debian 13 の base guest と操作の道具を作成。`guest.sh` は小さな sh 入口から Python の controller を呼ぶ（QMP JSON と座標変換を shell escaping 無しで扱う）。依存 package は既存 host にあり、host package 追加なし。

- `timeout 600 sh .../build-guest.sh`: exit 0。mmdebstrap 121.9975 秒、raw ext4 8 GiB。mesa-vulkan-drivers 25.0.7-2+deb13u1 / libvulkan1 1.4.309.0-1 / weston 14.0.2-1 / linux-image-amd64 6.12.107-1。guest kernel 6.12.107+deb13-amd64。
- `timeout 200 .../guest.sh start`: 180 秒以内に `guest: ready`。root と kei の loopback SSH 成功。kei の audio/video/input/kvm/render/netdev、`/run/user/1000` を確認。
- DRM card0 / ALSA controlC0 / event0〜5、Vulkan llvmpipe、mac80211_hwsim wlan0/wlan1、ALSA `'Master'` を確認。
- QMP PNG の login prompt を目視・ユーザーに提示。png-probe の size `1280 800`、2 点の色 `#000000`。key / click / type が QMP error 無し。
- run2 / port2226 の同時起動・SSH・停止 PASS。両 guest 停止後 overlay 無し、base image の size / mtime は一致。build の再実行は既存 image を保持。
- 追加の file 転送 / install 確認: 専用 stage の probe.txt を guest に install し、get 後 cmp 一致。guest 停止済み。host の `/opt` は変更していない。
- sh syntax、Python compile、`git diff --check` PASS。Master Tools 登録済み。共通 product code の変更無し、zedBSD 回帰対象無し。

証拠: `build/ws105-p001/`（build.log、start.log、verify.log、devices.txt、user.txt、image-before/after.txt、received.txt）。永続 PNG・版・試験 summary は `plan/history/ws105/q523/`。gdm variant の実行は p009、compositor / app の動作は後続 Phase。console / serial log は読んでいない。


実装 commit: `39a0941c272ae3da5091efcba14907610ba5683b`（WIP）。終了 UTC: 2026-10-01T06:04:15.180248+00:00。GitHub は未公開、Phase / WS event と intended close は outbox に保持。
