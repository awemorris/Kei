# WS104・WS105: zedBSD の build と回帰の command（そのまま実行する）

2026-10-01 に Q1 の survey（`make -n` と script の読み取り）で確かめた。**command は repo の root（`/home/awe/zedBSD-claude1` か、その worktree の root）から、書いてあるとおりに実行する。**
`<W>` は Phase ごとの作業の名前（例 `ws104-p004`）に置き換える。作業の出力は全て `build/<W>/` と `build/<W>-*/` に置く（他の agent・他の Phase と衝突しないように）。

## 0. 守ること（全て）

- **image の build（`disk-image`、`build-*-image.sh`）を 2 つ同時に走らせない。** `build/packages/` と `build/amd64/packages/toolchain` は全ての BUILD で共有。
- 試験の image を作る script（`build-criteria-image.sh` など）は、引数を省くと `build/amd64` に作り、既定の `build/amd64/hdd-image.img` を上書きする。**必ず BUILD の引数を渡す。**
- `plan/tools/boot-test.sh` は `OUTPUT` の directory を `rm -rf` する。**必ず専用の `OUTPUT` を渡す。**
- `criteria.sh` は guest の runtime を `build/ws035-sq-run` に固定し、`volume-p004.sh`・`volume-p005.sh` は `build/ws100-run` に固定している。これらは同時に 1 つだけ。
- toolchain の lock（`plan/tools/toolchain-lock.sh`）は外さない。普通の build・sysroot の作り直しは lock のままで通る（2026-10-01 確かめ）。
- QEMU の console・serial の log で判定しない。判定は下の PASS の行・終了の code・PNG。

## 1. build（amd64 の disk image）

```
mkdir -p build/<W>
make -j16 disk-image > build/<W>/build.log 2>&1; echo "make exit=$?"
grep -E ':[0-9]+:[0-9]+: warning:' build/<W>/build.log | grep -vE '/packages/|^\.\./src/|userland/base/noct/noct/' | wc -l
ls -la build/amd64/hdd-image.img
```

- PASS: `make exit=0`、2 行目の数が `0`、image がある。
- 我々の source は全て `-Wall -Wextra -Werror` で compile されるので、warning は build の失敗になる。2 行目の filter は外部 package（clang・openssh・libcxx・openssl・Noct）の warning を除くため。
- `make` の goal を省いても `disk-image`（`Makefile:37`）。`make image`・`make hdd-image` は無い。`BUILD` の既定は `build/amd64`。
- 時間: header（sysroot）を変えた後は、sysroot・全ての desktop の object・外部 package の作り直しで数分〜15 分程度（未計測）。変えていなければ 1 分以内。

## 2. sysroot だけを作り直す・中身の hash

```
make -j16 sysroot-amd64
mkdir -p build/<W>
(cd build/amd64/sysroot/usr/include && find . -type f | LC_ALL=C sort | xargs sha256sum) > build/<W>/sysroot-include.txt
```

- `make build/amd64/sysroot/.zedbsd-sysroot-complete` は**使わない**（規則の target は絶対 path なので、相対 path では「Nothing to be done」になる）。

## 3. 1 つの library・program だけを build

```
make -j16 build/amd64/dynamic/libkeiland.so
make -j16 build/amd64/bin/wayland
```

image は作り直さない。boot test・guest の試験の前には 1 の `disk-image` を走らせる。

## 4. boot test

```
OUTPUT=build/<W>/boot plan/tools/boot-test.sh build/amd64/hdd-image.img; echo "exit=$?"
```

- PASS: `exit=0` と、最後の方の行 `boot-test: PASS build/<W>/boot/login.png`。PNG（`build/<W>/boot/login.png`）をユーザーに見せる。
- 約 30 秒。GPU も KVM も要らない。

## 5. compositor の基準（WS099 の C1・C2・C9、QEMU の Venus）

```
sh plan/ws099/tests/build-criteria-image.sh build/<W>-criteria > build/<W>/criteria-build.log 2>&1; echo "exit=$?"
sh plan/ws099/tests/criteria.sh build/<W>-criteria/hdd-image.img build/<W>/criteria C1 C2 C9
cat build/<W>/criteria/results.txt
grep -c ' FAIL ' build/<W>/criteria/results.txt
```

- PASS: build の `exit=0`、results.txt の全ての行が `PASS`、最後の grep が `0`。行の形は `<Cn> <名前> <PASS|FAIL> seconds=N <詳細>`。criteria.sh 自体は常に exit 0 なので results.txt で判定する。
- criteria.sh は試験ごとに guest を起動し直す（前もって guest を起こさない）。約 20〜25 分。
- host の準備（host の起動ごとに 1 回。2026-10-01 は済み）: `sudo modprobe vgem && sudo chmod 0666 /dev/dri/renderD128`。

## 6. GPU の境界の試験（WS103 の物）

```
sh plan/tools/gpu-boundary/v1-check.sh build/amd64
sh plan/tools/gpu-boundary/run-dedicated-host.sh
sh plan/tools/gpu-boundary/run-gpu-zedbsd-host.sh
sh plan/tools/gpu-boundary/build-forge-image.sh build/<W>-forge > build/<W>/forge-build.log 2>&1; echo "exit=$?"
sh plan/ws035/tests/zdesktop-guest.sh start build/<W>-forge/hdd-image.img
sh plan/ws035/tests/zdesktop-guest.sh wait --timeout 240
sh plan/tools/gpu-boundary/forge-guest.sh build/<W>/forge
sh plan/tools/gpu-boundary/fence-guest.sh build/<W>/fence
sh plan/ws035/tests/zdesktop-guest.sh stop
```

- PASS: `v1-check: PASS`、`dedicated-host: PASS`（2 回、ordinary と sanitize）、`gpu-zedbsd-host: PASS`（2 回）、`forge-guest: PASS`、`fence-guest: PASS`。
- `zdesktop-guest.sh start` はすぐ戻るので、必ず `wait` を挟む（`guest: ready` が出る）。

## 7. glass の見た目の試験（zdesktop-p059）

```
sh plan/ws035/tests/zdesktop-guest.sh start build/<W>-criteria/hdd-image.img
sh plan/ws035/tests/zdesktop-guest.sh wait --timeout 240
sh plan/ws035/tests/zdesktop-p059.sh build/<W>/p059
sh plan/ws035/tests/zdesktop-guest.sh stop
```

- PASS: 最後の行 `p059: PASS`。**これは pen（tablet）の試験ではない**（titlebar・hover・drag の試験で、pointer は QEMU の usb-tablet の絶対座標）。
  pen（tablet の protocol）の試験は `plan/ws079/tests/notes-pen.sh`（notes の image）。使い方は script の先頭。

## 8. Settings と音

```
sh plan/ws089/tests/host-build.sh
sh plan/ws089/tests/build-settings-image.sh build/<W>-settings > build/<W>/settings-build.log 2>&1; echo "exit=$?"
GUEST_RUNTIME=$PWD/build/<W>-settings-run sh plan/ws089/tests/settings-guest.sh start build/<W>-settings/hdd-image.img
GUEST_RUNTIME=$PWD/build/<W>-settings-run sh plan/ws089/tests/settings-regress.sh build/<W>/settings-regress
GUEST_RUNTIME=$PWD/build/<W>-settings-run sh plan/ws089/tests/settings-guest.sh stop
sh plan/ws100/tests/host-audio.sh
sh plan/ws100/tests/build-volume-image.sh build/<W>-volume > build/<W>/volume-build.log 2>&1; echo "exit=$?"
sh plan/ws100/tests/volume-p004.sh build/<W>-volume/hdd-image.img build/<W>/volume-p004
sh plan/ws100/tests/volume-p005.sh build/<W>-volume/hdd-image.img build/<W>/volume-p005
```

- **音量の image の前提**: `volume-p004.sh`・`volume-p005.sh` は `build/ws100-tests/audiod-feedback` を要る（`config-amd64-volume.mk:8`、無いと image に入らず試験が落ちる）。2026-10-01 の survey 時点の main には無かった。WS104 p008 でこの guide の compile / link command で作成して volume 回帰を実行済み。作り方は [WS100 の guide.md](../../ws100/guide.md) §5.1。
- PASS: `built build/ws089-host/settings-render`（exit 0）、`settings-regress: PASS`（約 10 分）、`host-audio: N/N passed`（exit 0）、`volume-p004: PASS`・`volume-p005: PASS`（各 6〜7 分）。

## 9. 時間の目安（WS104 の Phase の回帰の全部）

build（〜15 分）+ boot test（1 分）+ criteria の image と C1・C2・C9（〜30 分）+ forge の image と試験（〜10 分）。Settings・音を加えると +30 分。
