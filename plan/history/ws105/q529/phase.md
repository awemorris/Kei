<!-- awesome-plan project=zedbsd record=ws105-p006 -->

# ws105-p006: compositor の Linux の build と module (1): seat-direct・入力・session（wl_shm の client まで）

Status: uncleared
Disposition: normal
Parent: [WS105](../../../ws105/ws.md)
Queue: q529 / q529-i01
依存: p005、**WS104 の完了**（compositor の `zwl-gpu.h`・`zwl-input.h`・`zwl-os.h`（7 つの hook）と `zedbsd/` の module）
実行者: phase-runner（high）。**始める前に [design.md](../../../ws105/design.md) の §5（特に §5.1・§5.3・§5.4・§5.6）を読む**

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
sh $G click 23 17; sleep 2; sh $G screenshot $PWD/build/keiland-linux/p006-home.png
sh $G key esc
```

- compositor の引数は zedBSD の `/etc/keiland/session`（`userland/desktop/wayland/session.sh`）と同じ（`--session --glass`。無いと 150 秒で終わる、design §5.1）。`ZWL READY` の行は compositor
  の stdout の log で、判定に使ってよい（compositor 自身の log。guest の serial の log ではない）。
- App Home は `plan/ws099/tests/c5-transitions.sh` と同じ top-left launcher click（23,17）で開く。key の確かめは Esc の閉鎖と Super+Tab の Wiseview（super は QKeyCode の `meta_l`）。当初手順の「meta単独でHome」はactual sourceに無い操作だったため修正（scope・入力の受け入れは同じ）。
- 画面の大きさは `png-probe.py --size` で確かめ、座標をその中に取る。

## 完了の条件

1. build（gcc・clang）、`elf-check: PASS`、`makefile-sync: PASS`、`header-check: PASS`。
2. guest: compositor が `ZWL READY` を出す。screenshot に wallpaper と上の system bar が出る（`png-probe.py` で system bar の帯の中の点と wallpaper の中の点の色が違う。PNG をユーザーに見せる）。
3. guest: `wlshm` の窓が出る（`p006-wlshm.png` と `p006-desktop.png` で窓の位置の画素が違う）。
4. 入力: `p006-move1.png` と `p006-move2.png` で cursor の位置が変わる（2 つの PNG の差が cursor の付近にある）。App Home が開く（`p006-home.png`）。
5. Log Out（App Home の Log Out の項目を `click`）で compositor が終わり、`chvt 1` の後の screenshot に console の文字。VT が戻り、key が console に届く。
6. SIGTERM（`sh $G ssh 'pkill -TERM -x wayland'`）でも console に戻る。
7. `KEILAND_SEAT` を付けずに SSH から起動すると、`XDG_SESSION_TYPE=tty` なので `direct` を選ぶ（design §5.1 の規則の確かめ）。
8. zedBSD の回帰（design §9.2、[WS104 の commands.md](../../../ws104/commands.md) の §1・§4・§5・§6、`keiland-os-boundary/check.sh`）。

## 結果

uncleared（q529-i01）。Linux compositor/direct seat/evdev/VT/handoff/GPUなしglobalと、独立Makefile・fonts・wlshmを実装した。gcc/clang exit0 warning0、12ELF / source-sync / 126header / scoped style PASS。

- guest: READY、Virtual-1 1280×800。wallpaper/bar (100,10)=#ebf9ec / (100,400)=#c5dde6。wl_shm center (640,400) background (224,235,248)→(0,0,255)、約6000frame表示。cursor差は(200,200)と(900,600)の付近。App HomeとEsc、Super+TabのWiseviewを確認。
- 手順のmeta単独でHomeという記述は実装と違った。既存c5-transitions.shと同じlauncher click (23,17) に訂正、keyboardはEsc/Super+Tabで確認。受け入れ範囲は同じ。
- WiseviewをEscで閉じる連続描画で `ZWL VULKAN_ERROR operation=submit result=-1000001004`、`ZWL FAILED site=compose_draw errno=5`、EXIT frames=6051 error5 cleanup_failed1。masterはseatのroot fdで保持。KMSが100ms総期限をOUT_OF_DATEとする実装を確認、設計は各pollの上限100msであり総期限の意図ではなかった。タイミングからpoll deadlineが原因と推定、次のp005修正Queueでbounded遅延試験により確かめる。
- Log Outは上記でcompositorが先に終了したため未達。この終了をLog Out成功とは数えない。独立したSIGTERM終了とseat未指定のSSH tty sessionではREADY→EXIT error0、chvt1文字とconsole key 'kei' PNGを確認。
- zedBSD disk-image exit0、C1〜C5 boundary、v1、dedicated/decode host、boot login PNG確認。C1/C2/C9＋forge/fenceの承認済み回帰processは進行中（exec session52085、outputs build/ws105-p006）。既存承認の検証だけを継続して証拠を保存し、再開Queueでterminal結果を確認する。これらを今PASSと扱わない。
- Linux guest stop済み、overlay廃棄。sourceはWIPに保存。mainの委任された技術判断でp005を再開し、poll1回≤100ms＋有限の総期限のKMS待機に直す。p005の修正と再検証後、同じp006を再開してLog Out・SIGTERM・keyboard・zedBSD回帰の全条件を確認する。WS105の受け入れは変更しない。


実装 commit: `80eea509710d51928bf14efd1246c02c80da38f4`（WIP）。終了 UTC: 2026-10-01T08:25:21.947014+00:00。GitHub は未公開、Phase / WS event と intended close は outbox に保持。
