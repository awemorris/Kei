<!-- awesome-plan project=zedbsd record=ws004-p051 -->
# ws004-p051: BUG-134 AX211 の起動停止と不動作を passthrough で解析・修正

Status: uncleared（q590-i01、passthrough で再現せず、ユーザーの実機の切り分け待ち）
Disposition: normal
Parent: [WS004](../ws.md)
Bug: [BUG-134](../../bugs/BUG-134.md)
Queue: q590 / q590-i01（P1）

## 範囲

1. 5330 host（`solaris10-man`）で AX211（`0000:00:14.3`）を iwlwifi から vfio-pci へ一時的に付け替え、`CONFIG_DRIVER_PCI_INTEL_AX211 := y` の image を passthrough の QEMU で起動し、起動停止を再現する。既存の `plan/ws004/tests/run-intel-ax211-vfio-qemu.sh` を使う／直す。
2. 停止を QEMU の gdbstub・monitor・QMP で解析する（console/serial log で判定しない）。いつから壊れたか（git の履歴、ws004-p038 系の以前の到達点）も調べる。
3. 原因が `src/drivers/wifi/intel-ax211/`・firmware の package・その driver が使う kernel の部分にあれば修正する。HAL の API（`include/hal/hal.h`）の変更が要るなら差分を plan に置いて止める。
4. 修正後: driver 有効の image が passthrough で起動して login に至り、AX211 の device が attach する（scan・接続まで確かめられれば記録、無理なら未実施と書く）。driver 有効の image で `plan/tools/boot-test.sh`（passthrough 無し）。build warning 0。新しい code は C 全文規約。

## host の制約

- 付け替えるのは `0000:00:14.3` だけ。終わったら iwlwifi へ戻す。host の USB の LAN、iGPU の mode、他の QEMU/VM、host の reboot・kernel・package には触れない。
- 5330 host の試験は `/tmp/i915-hw.lock`（centris）を flock で取ってから行う。

## 受け入れ

- 起動停止の原因の特定（証拠つき）と修正、上の 4 の確認。実機の単独起動（USB）はユーザーの確認で、未実施と書く。

## 結果

### q590-i01（P1、2026-10-02、base `257b2bd9f`）

結論: **5330 の VFIO passthrough では起動停止を再現できなかった**。AX211 の attach・firmware・scan まで passthrough で動いた。
起動停止は passthrough に無い条件（素の 5330 の実機）に依存すると推定するが、証拠は無い。2026-10-02 Q1 経由のユーザー決定
「ユーザーが実機でテストして切り分けてから再開」により解析を止め、実機の試験用の image を作った（下）。原因は未特定、修正は無い。

#### 再現の試み（QEMU passthrough の証拠。判定は画面・gdbstub・monitor。console/serial/debugcon の log は読んでいない）

5330 host（`solaris10-man`）で `/tmp/i915-hw.lock` を取り、`plan/ws004/tests/run-intel-ax211-vfio-qemu.sh` で
`0000:00:14.3` を vfio-pci に付け替えて起動した。どの回も終了後に script が iwlwifi へ戻した（`AX211-VFIO: restored 0000:00:14.3 to iwlwifi`、
driver_override `(null)`、route は `enx6c1ff71a08b6`、iGPU は元の vfio-pci のまま、QEMU 0 個、lock 解放を確認）。

| run | image（config） | 渡した device | 結果 |
| --- | --- | --- | --- |
| 1 | `build/p1-ax211`（`tests/config-ax211-vfio.mk`: ws045 base + AX211=y + `intelax211-firmware`、graphical boot） | AX211 | 画面で login まで到達。root で login し `ifconfig -a` に `wlan0`（UP）、`wifi wlan0 status` は scan=complete error=0、`wifi wlan0 list` は 36 件。画面に `intel-ax211: master-disable indication timed out; PCI bus master disabled and reset completed`（firmware boot の成功の経路の情報の行） |
| 2 | `build/p1-desk`（`tests/config-ax211-desktop.mk`: demo の config + AX211=y + firmware、`I915_TEST_VBT=y`） | AX211 + iGPU `00:02.0`、4 GiB | 全 vCPU が `hal_cpu_idle`（停止ではない） |
| 3 | `build/p1-desk-g`（2 と同じ、kernel だけ DWARF 付き `-flto -g`） | AX211 + iGPU、4 GiB | gdb で全 process/thread を辿った（`tests/ax211-gdb-threads.py`）: init・syslogd・audiod・networkd・cron・sessiond・`/bin/wayland`・`/bin/files` が動作、kernel thread は全て正常な待ち。`ax211_controllers`: ready=1、runtime_active=1、net_live=1（firmware 起動済み） |
| 4 | `build/p1-main`（`tests/config-ax211-main.mk`: main の `config.mk` の写し + AX211=y、VBT 無し、DWARF 付き） | AX211 + iGPU、4 GiB、12 vCPU（実機と同じ数） | 画面で console の `login:` まで到達（VBT が無いので greeter は終わり getty へ。期待どおり）。runtime_active=1。login 後 `wifi wlan0 down` は 0 で終了 |

passthrough 無しの `plan/tools/boot-test.sh`（NVMe、PNG で login prompt を判定）: `build/p1-ax211`・`build/p1-desk`・`build/p1-main` の 3 つとも PASS
（`build/p1-*-boot/login.png`、worktree の中、git に入れない）。

#### 分かったこと（証拠つき）

- **「AX211 が動かない」の一つの原因は構成**: demo の image（`plan/ws075/demo/config-demo-hdmi.mk` → `plan/ws031/tests/config-zdesktop-hw.mk` →
  `plan/ws035/tests/config-amd64-userland.mk`）は `CONFIG_DRIVER_PCI_INTEL_AX211 := y` だが `intelax211-firmware` を含まない
  （`config-amd64-userland.mk` の見出し「without firmware downloads」）。main の `build/demo-lcd9`・`demo-lcd-pt`・`demo-takeover` の vmunix には
  driver の symbol があり、rootfs に `/lib/firmware/intel` が無い。firmware 無しでは wlan0 は up できない。逆に main の `config.mk` は firmware を持つが AX211=n。
  これらの config は WS035/WS075 の所有で、この Phase では変えていない（main に報告）。
- 実機と passthrough の差（未検証の候補、記録のみ）: 実機では AX211 の BAR0 `0x6055294000` が xHCI `0x6055260000`・HDA `0x6055290000`・MEI などと
  同じ 2 MiB の範囲に並ぶ（passthrough の guest では AX211 は `0x7011000000` に単独）。実機の AX211 の INTx は IRQ16 を HDA・SMBus・thermal・TBT DMA と共有する。
  実機の CNVi は VFIO の FLR を経ずに UEFI の状態から始まる。どれも原因である証拠は無い。
- いつ壊れたか: AX211 の driver の source の最後の意味のある変更は 2026-09-12（`c98c46064`）、2026-09-26 の `1e867fbfd` は主に名前の整理。
  ws004-p038 の到達点（q066）は VFIO だけで、素の 5330 の単独起動は p038 でも未実施のまま（p038 の Status）。実機で一度でも動いた記録は plan に無い。

#### 作った・直した道具（`plan/ws004/tests/`）

- `run-intel-ax211-vfio-qemu.sh`: 任意の `AX211_VFIO_IGPU_BDF`（vfio-pci 所有を確かめるだけで付け替えない、memfd と 39-bit の制限）、
  `AX211_VFIO_MEMORY`・`AX211_VFIO_SMP`・`AX211_VFIO_GDB_PORT`（host の loopback のみ）・`AX211_VFIO_GDB_WAIT`。
- `ax211-vfio-hmp.py`（monitor の socket へ HMP の command、`quit` に対応）、`ax211-gdb-threads.py`（gdb の `zthreads`: 全 process/thread と
  寝ている thread の backtrace を `asm_task_dispatch` の保存 frame から）。
- config: `config-ax211-vfio.mk`・`config-ax211-desktop.mk`・`config-ax211-main.mk`・`config-ax211-usb.mk`（実機の試験用、下）。

#### 実機の試験用の image（Q1 指示 2026-10-02、ユーザーが USB から起動）

- config: `plan/ws004/tests/config-ax211-usb.mk` = `config/ci/config-amd64.mk`（amd64、board pcat、AX211=y、`intelax211-firmware` を既に含む、
  graphical boot、i915・HDA・ACPI）から clang・libcxx・remacs だけを除いたもの（新しい tree で何時間もかかるため）。`I915_TEST_VBT` は無し（既定 n）。
  Q1 の指示の `config/ci/config-pcat.mk` は i386 で AX211=n のため、5330（amd64）の CI 構成である `config-amd64.mk` を使った。
- build: `make -j32 ZEDBSD_CONFIG=plan/ws004/tests/config-ax211-usb.mk BUILD=build/p1-usb disk-image`（rc=0、zedBSD の source の warning 0。
  warning は OpenSSH の third-party の deprecated と Noct の 1 件だけ）。
- 成果物（worktree の `build/`、git に入れない）: `build/p1-usb/hdd-image.img`（2216689664 byte、SHA-256
  `530670e9269b38af05862ca7784edcd8362d3be49db2348ca7b91a477e59cf9c`）、`build/p1-usb/hdd-image.img.gz`（37254490 byte、SHA-256
  `8260d61740482c1f96d29d9174523848021dbef809ad7bc184969ecc6077abe5`、`gzip -t` OK）。boot の行: `kernel=vmunix`、`rootpart=PARTLABEL=zedBSD-root`、
  `swap0=PARTLABEL=zedBSD-swap`、`logo=logo.ppm`、`login=graphical`、`kmsg=quiet`。vmunix に AX211 の symbol 90 個、rootfs に
  `/lib/firmware/intel/iwlwifi/iwlwifi-so-a0-gf-a0-89.ucode`・`.pnvm`、`/sbin/wifi`・`/sbin/net`・`/usr/sbin/sshd`。
- `plan/tools/boot-test.sh`（TCG、NVMe）: 3 回とも boot-test.py の QMP `screendump` が 30 秒応答せず TimeoutError（画面は splash の途中）。
  対照の同じ config の AX211=n の image（`build/p1-usb-noax`）も同じ TimeoutError なので AX211 とは無関係（OpenSSH の初回の host key 生成が
  TCG で重いことと一致。未確定）。同じ image の DWARF 版を手で TCG の QEMU で起動し、gdb で `login`（console の tty の read で待つ）・sshd・networkd 等の
  process を確認し、画面（screendump）でも `init: started getty_console` と `login:` を確認した。boot-test.sh の PASS は**未取得**。

ユーザーへの手順（Q1 経由で渡す）:

1. 書き込み: `zcat hdd-image.img.gz | sudo dd of=/dev/sdX bs=4M conv=fsync status=progress`（`sdX` は USB の device。Windows なら
   Rufus / balenaEtcher で `hdd-image.img` を書く）。
2. 5330 で F12 の一回の起動 menu から USB（UEFI）を選ぶ。
3. 期待: Kei の splash の後に greeter（または i915 の画面）。**splash のまま 2 分以上進まない、黒い画面、固まる**ときは、その画面を写真に撮る。
   次に USB の ESP（FAT）の `/zedbsd.cfg` から `kmsg=quiet` の行を消して（他の行は足さない・重ねない）もう一度起動し、止まった画面（kernel の message）を撮る。
4. 起動したら root/root で login（greeter から Terminal、または同じ LAN から `ssh root@IP`、password `root`）。
5. WiFi の確認（SSID・password は記録に書かない）:
   `dmesg | grep -i -E 'ax211|wlan'` → `ifconfig -a`（`wlan0` があるか）→ `wifi wlan0 up` → `wifi wlan0 search start` → 数秒後 `wifi wlan0 list` →
   `net wifi set-key 'SSID' 'PASSPHRASE'` → `net wifi enable` → `net wifi connect 'SSID'` → `ifconfig wlan0`（IPv4）→ `ping -c 3 GATEWAY` → `ping -c 3 8.8.8.8`。
   うまくいかない段で止め、その段の出力と `dmesg | grep -i ax211` を写真か text で返す。

#### 未実施

- 起動停止の再現・原因の特定・修正（passthrough では再現しない。実機はユーザー）。
- 実機（素の 5330）での起動・attach・scan・接続（ユーザーの確認待ち）。
- passthrough での WPA2 接続・DHCP（今回は scan まで。接続は p038 の q066 で確認済みの経路）。
- 新しい C の code は無いので build の warning の関門・C 規約の確認は対象外（image の build の warning は 0、Noct の third-party の 1 件を除く）。

#### 再開の条件

ユーザーの実機の結果（どの画面で止まるか、kernel の message が見える image での写真、または `dmesg`）が届いたら、それに合わせて passthrough
か実機の証拠で解析を再開する。
