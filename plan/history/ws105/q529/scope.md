<!-- awesome-plan project=zedbsd record=ws105-p006 -->

# ws105-p006: compositor の Linux の build と module (1): seat-direct・入力・session（wl_shm の client まで）

Status: planned
Disposition: normal
Parent: [WS105](../ws.md)
Queue: なし
依存: p005、**WS104 の完了**（compositor の `zwl-gpu.h`・`zwl-input.h`・`zwl-os.h`（7 つの hook）と `zedbsd/` の module）
実行者: phase-runner（high）。**始める前に [design.md](../design.md) の §5（特に §5.1・§5.3・§5.4・§5.6）を読む**

## 目的

compositor（`userland/desktop/wayland/`、program `wayland`）を Linux で build し、guest の VT で root で起動して、wallpaper・system bar・wl_shm の client の窓を出し、
pointer と key を届ける。GPU の client の buffer（`zwp_linux_dmabuf_v1`）は p007。

## 作る・変える file

| file | 中身 |
| --- | --- |
| `userland/desktop/wayland/Makefile.linux` | program `wayland`（`bin`）。source は**明示の一覧**: zedBSD の `userland/desktop/wayland/Makefile` の `KEILAND_SOURCES` の中から `$(KEILAND_ZEDBSD_SOURCES)` を除いた物を 1 つずつ書き写す（`vkdemo/display.c`・`artwork/mark.c`・`picture/color-glyph.c` は `KEILAND_SOURCES` に既に入っている。二重に書かない）と、`linux/*.c`。zedBSD の `Makefile` を include しない（D2）。依存と system の library は design §3.3 の表。font の data（zedBSD の `KEILAND_FONT_DATA` と同じ対応、`share/fonts/`・`share/licenses/keiland-fonts/`）と emoji の font（design §3.2）を `KEILAND_LINUX_DATA` で。`keiland-x11` は入れない（xserver は範囲の外、D21）。`# keiland-linux-sync: skip userland/desktop/wayland/zedbsd/*` |
| `wayland/linux/seat-linux.h` | linux の file の間の内部の宣言（seat の open・close、device の open・close、DRM の fd と path） |
| `wayland/linux/os-linux.c` | `zwl-os.h` の Linux の実装（design §5.1）: seat の選択（この Phase は `direct` だけ）、`setenv("KEILAND_DRM_DEVICE", path, 1)`、VT の `KD_GRAPHICS`・`K_OFF` と戻し、既定の socket、`zwl_os_display_acquire`（`vkAcquireDrmDisplayEXT`）・`zwl_os_display_release`（`vkReleaseDisplayEXT`）、poll は 0 個 |
| `wayland/linux/seat-direct-linux.c` | root で DRM と入力の device を直接 open する seat |
| `wayland/linux/input-linux.c` | `zwl-input.h` の Linux の実装（design §5.3、`EVIOCSCLOCKID(CLOCK_MONOTONIC)`）。`zedbsd/input-zedbsd.c` と同じ形で書き、device の open・close は seat の関数 |
| `wayland/linux/handoff-linux.c` | `zwl_handoff_*`（design §5.4） |
| `wayland/linux/gpu-linux.c`（この Phase は空の形） | `zwl_gpu_global_interface()` → NULL（GPU の global を出さない）、`zwl_gpu_request` → `EPROTO`、`zwl_gpu_commit` は何もしない、instance の拡張は `VK_EXT_direct_mode_display`・`VK_EXT_acquire_drm_display`、device の拡張は 0、frame の fence は 0（D22） |
| 共通: `protocol.c` | `zwl_gpu_global_interface()` が NULL なら GPU の global を registry に出さない（`registry_events` で飛ばし、`bind_global` で EPROTO）（design §5.6） |
| 共通: `main.c` | `--socket` が与えられたかの flag（`server.socket_given`）（design §5.6）。`zwl_os_open` が見る |
| 共通: `titlebar-shell.c:1299` | `float colour[4] = { 0.0f, 0.0f, 0.0f, 0.0f };`（gcc の誤検出、design §5.6） |
| `userland/desktop/wlshm/Makefile.linux` | 試験の client（wl_shm だけ、依存 `libwayland-client.so`） |
| `plan/tools/gpu-boundary/v1-check.sh`（main） | `find $dir` が `linux/` を拾わないように（`find $dir -path "$dir/linux" -prune -o -name '*.[ch]' -print`）。linux の file は zedBSD の sysroot で compile できない |
| `userland/desktop/keiland-linux.mk` | `KEILAND_LINUX_PACKAGES` に `userland/desktop/wayland/Makefile.linux`・`userland/desktop/wlshm/Makefile.linux` |

**compositor の code に `#if defined(__linux__)` を書かない**（`zwl-evdev.h` の外）。OS で違う所は `linux/` の file の関数にする。design §5.6 の外の共通の file の変更が要るなら止めて main に報告。
共通の file を変えたので zedBSD の回帰（design §9.2）を流す。

## 手順

```
make -j64 keiland-linux && make keiland-linux-install DESTDIR=$PWD/build/keiland-linux/stage
make -j64 keiland-linux CC=clang KEILAND_LINUX_BUILD=build/keiland-linux-clang
sh plan/tools/keiland-linux/elf-check.sh build/keiland-linux/stage && sh plan/tools/keiland-linux/makefile-sync.sh && sh plan/tools/keiland-linux/header-check.sh
G=plan/tools/keiland-linux/guest.sh
sh $G start && sh plan/tools/keiland-linux/install-guest.sh
sh $G ssh 'rm -f /tmp/wayland.log; openvt -c 7 -s -- sh -c "KEILAND_SEAT=direct /opt/keiland/bin/wayland --session --glass --socket=/run/keiland-0 > /tmp/wayland.log 2>&1"'
sh $G ssh 'for i in $(seq 30); do grep -q "ZWL READY" /tmp/wayland.log && break; sleep 1; done; head -30 /tmp/wayland.log'
sh $G screenshot $PWD/build/keiland-linux/p006-desktop.png
sh $G ssh 'XDG_RUNTIME_DIR=/run WAYLAND_DISPLAY=keiland-0 nohup /opt/keiland/bin/wlshm --size=480x320 --frames=6000 > /tmp/wlshm.log 2>&1 &'
sleep 3; sh $G screenshot $PWD/build/keiland-linux/p006-wlshm.png
sh $G move 200 200; sleep 1; sh $G screenshot $PWD/build/keiland-linux/p006-move1.png
sh $G move 900 600; sleep 1; sh $G screenshot $PWD/build/keiland-linux/p006-move2.png
sh $G key meta_l; sleep 2; sh $G screenshot $PWD/build/keiland-linux/p006-home.png
sh $G key esc
```

- compositor の引数は zedBSD の `/etc/keiland/session`（`userland/desktop/wayland/session.sh`）と同じ（`--session --glass`。無いと 150 秒で終わる、design §5.1）。`ZWL READY` の行は compositor
  の stdout の log で、判定に使ってよい（compositor 自身の log。guest の serial の log ではない）。
- App Home を開く key は zedBSD の試験と同じ物を `plan/ws099/tests/` の script で確かめて使う（super は QKeyCode の `meta_l`）。
- 画面の大きさは `png-probe.py --size` で確かめ、座標をその中に取る。

## 完了の条件

1. build（gcc・clang）、`elf-check: PASS`、`makefile-sync: PASS`、`header-check: PASS`。
2. guest: compositor が `ZWL READY` を出す。screenshot に wallpaper と上の system bar が出る（`png-probe.py` で system bar の帯の中の点と wallpaper の中の点の色が違う。PNG をユーザーに見せる）。
3. guest: `wlshm` の窓が出る（`p006-wlshm.png` と `p006-desktop.png` で窓の位置の画素が違う）。
4. 入力: `p006-move1.png` と `p006-move2.png` で cursor の位置が変わる（2 つの PNG の差が cursor の付近にある）。App Home が開く（`p006-home.png`）。
5. Log Out（App Home の Log Out の項目を `click`）で compositor が終わり、`chvt 1` の後の screenshot に console の文字。VT が戻り、key が console に届く。
6. SIGTERM（`sh $G ssh 'pkill -TERM -x wayland'`）でも console に戻る。
7. `KEILAND_SEAT` を付けずに SSH から起動すると、`XDG_SESSION_TYPE=tty` なので `direct` を選ぶ（design §5.1 の規則の確かめ）。
8. zedBSD の回帰（design §9.2、[WS104 の commands.md](../../ws104/commands.md) の §1・§4・§5・§6、`keiland-os-boundary/check.sh`）。

## 結果

（実行の後に書く）
