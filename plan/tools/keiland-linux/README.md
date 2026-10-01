# Keiland Linux の試験の道具

WS105 の Debian 13 guest を作り、loopback SSH と QMP で操作する。host の画面・入力 device は使わない。host に `/opt/keiland` を install しない。
SSH は `127.0.0.1:2225` → QEMU guest の port 22。試験専用の鍵は ignored build directory に保存する。
全ての試験 command に `timeout` を付ける。host の Vulkan 試験は `KEILAND_DRM_DEVICE=none` と `env -u WAYLAND_DISPLAY -u DISPLAY` を使う。

## Guest の作成・起動・確認

repo root で実行する。

```sh
timeout 600 sh plan/tools/keiland-linux/build-guest.sh
timeout 200 sh plan/tools/keiland-linux/guest.sh start
timeout 30 sh plan/tools/keiland-linux/guest.sh ssh 'uname -r; ls /dev/dri/card0 /dev/snd/controlC0; ls /dev/input/'
timeout 30 sh plan/tools/keiland-linux/guest.sh ssh 'env -u WAYLAND_DISPLAY -u DISPLAY vulkaninfo --summary'
timeout 30 sh plan/tools/keiland-linux/guest.sh ssh 'modprobe mac80211_hwsim radios=2; iw dev'
timeout 30 sh plan/tools/keiland-linux/guest.sh ssh 'amixer -c 0 scontrols; id kei'
timeout 30 sh plan/tools/keiland-linux/guest.sh screenshot "$PWD/build/keiland-linux/console.png"
timeout 30 python3 plan/tools/keiland-linux/png-probe.py --size build/keiland-linux/console.png
timeout 30 python3 plan/tools/keiland-linux/png-probe.py build/keiland-linux/console.png 0 0 100 100
timeout 30 sh plan/tools/keiland-linux/guest.sh key a
timeout 30 sh plan/tools/keiland-linux/guest.sh click 100 100
timeout 45 sh plan/tools/keiland-linux/guest.sh stop
```

`build-guest.sh [--force] [OUT] [base|gdm]` は既存 image を再利用する。gdm の既定 OUT は `build/keiland-linux/guest-gdm`（OUT を空文字にして variant を指定）。共有 image の `--force` は main だけが利用中の guest が無いことを確認して使う。
image は raw ext4、8 GiB。mmdebstrap unshare mode なので sudo 不要。`packages.txt` に package の版を保存する。

## 操作

`guest.sh` は `guest.py` を呼ぶ。既定 `GUEST_DIR=build/keiland-linux/guest`、`GUEST_IMAGE=$GUEST_DIR/guest.img`、`GUEST_RUN=build/keiland-linux/run`、`SSH_PORT=2225`。
path は絶対 path に変換する。起動ごとに overlay を作り、停止後に消す。base image は読み取り専用。

| Command | 操作 |
| --- | --- |
| `start` / `stop` / `status` | SSH ready（180 秒上限）まで待つ / poweroff・QMP quit / process と SSH の状態 |
| `ssh 'COMMAND'` | root で command（既定 120 秒上限、`GUEST_COMMAND_TIMEOUT` で指定） |
| `put SRC DST` / `get SRC DST` | file を guest に送る / 受け取る |
| `screenshot PNG` | QMP screendump（絶対 path、RGB PNG） |
| `key ctrl alt f2` | 全 key を押し、逆順で離す。super は `meta_l` |
| `type 'text'` | 英小文字・数字・空白・`-`・`.`・`/` を入力 |
| `move X Y` / `click X Y [left\|right\|middle]` | screenshot の実寸で絶対座標に変換して入力 |

`SSH_USER=kei` で非 root の利用者を指定する。起動・停止には既定の root を使う。
別 guest は run directory と port を変える。

```sh
timeout 200 env GUEST_RUN="$PWD/build/keiland-linux/run2" SSH_PORT=2226 sh plan/tools/keiland-linux/guest.sh start
timeout 45 env GUEST_RUN="$PWD/build/keiland-linux/run2" SSH_PORT=2226 sh plan/tools/keiland-linux/guest.sh stop
```

## Install

`install-guest.sh [STAGE]`（既定 `build/keiland-linux/stage`）は `STAGE/opt/keiland` を guest の `/opt/keiland` に置き換える。
`STAGE/usr/share/wayland-sessions/keiland.desktop` があれば gdm の session file も送る。host の install tree は変えない。

```sh
timeout 180 sh plan/tools/keiland-linux/install-guest.sh build/keiland-linux/stage
```

## zedBSD の回帰

[zedBSD の検証手順](zedbsd-commands.md)（WS104 から移した）を参照する。Linux guest の boot 確認は SSH と PNG、zedBSD は `plan/tools/boot-test.sh`。
QEMU serial / console log を受け入れ判定に使わない。
