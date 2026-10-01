<!-- awesome-plan project=zedbsd record=ws105-p007 -->

# ws105-p007: compositor の Linux の module (2): `zwp_linux_dmabuf_v1` の server と implicit sync

Status: cleared
Disposition: normal
Parent: [WS105](../../../ws105/ws.md)
Queue: q532 / q532-i01
依存: p006
実行者: phase-runner（high）。**始める前に [design.md](../../../ws105/design.md) の §5.2・§5.6 と §4.8（client の側）を読む**

## 目的

compositor が Linux の GPU の client（libvulkan-compat の WSI を使う app）の buffer を受けて合成できるようにする（決定 D6・D17）。

## 作る・変える file

| file | 中身 |
| --- | --- |
| `wayland/linux/gpu-linux.c` | design §5.2 の全部: global `zwp_linux_dmabuf_v1` v3、bind の時の `format`・`modifier` の event、`create_params`、params の `add`・`create`（`created` は `zwl_create_server` で id を作る）・`create_immed`・`destroy`、検査と error、dma-buf の import（`zwl_import_adopt`）、log の行（zedBSD と同じ形）、`zwl_gpu_commit` の `DMA_BUF_IOCTL_EXPORT_SYNC_FILE(READ)` と `--log-frames` の `ZWL ACQUIRE_FENCE`、拡張の一覧（physical device にある物だけ）、`zwl_gpu_object_free`（buffer に残した fd を閉じる、params の記録を消す）、`zwl_gpu_bind`（bind の直後の format / modifier） |
| 共通: `zwl.h`・`protocol.c`・`objects.c` | design §5.6 の p007 の行: object の kind `ZWL_GPU_OBJECT`（`zwl_dispatch` で `zwl_gpu_request` へ）、`struct zwl_object` の `void *gpu_private`、`object_free` の中で `zwl_gpu_object_free(object)` を呼ぶ。`zwl-gpu.h` に両 hook と `zwl_gpu_bind` を宣言し、`bind_global` の factory 分岐で `zwl_gpu_bind(object)` を呼ぶ |
| `wayland/zedbsd/gpu-buffer-zedbsd.c` | `zwl_gpu_object_free` と `zwl_gpu_bind` の zedBSD の実装（何もしない）。`zwl_gpu_request` が `ZWL_GPU_OBJECT` を受けたら `EPROTO`（zedBSD では来ない） |
| 共通: `userland/desktop/mview/renderer.c` | `#include <stdio.h>`（design §5.6） |
| `userland/desktop/wltest/Makefile.linux` | Vulkan の WSI の試験の client（依存 `libwayland-client.so libvulkan.so.1`、`-lm`） |
| `userland/desktop/mview/Makefile.linux` | Model viewer（依存 `libwayland-client.so libvulkan.so.1`、`-lm`）と model の data（zedBSD の `mview/Makefile` の 13 番目の引数の `/usr/share/mview/...` を `share/mview/...` に、`KEILAND_LINUX_DATA`） |
| `plan/tools/keiland-linux/dmabuf-forge.c`（main が merge） | 下の確かめ 5 の偽の client |

細部:

- params の `add` の fd は `zwl_take_fd(client)`（wire の SCM_RIGHTS）で受ける。fd がまだ届いていなければ `EAGAIN`（zedBSD の factory と同じ）。
- 1 つの params は 1 回だけ使える（2 回目は `already_used`）。
- import の失敗（Vulkan の error）は、`create` なら `failed`、`create_immed` なら `invalid_wl_buffer` の protocol の error（client は終わる）。log は zedBSD と同じ `ZWL IMPORT_ERROR client=... errno=...`。
- server の `gpu_limits`（`compose_limits`）は Linux でも同じ意味（`maxImageDimension2D`）。

## 手順

```
make -j64 keiland-linux && make keiland-linux-install DESTDIR=$PWD/build/keiland-linux/stage
make -j64 keiland-linux CC=clang KEILAND_LINUX_BUILD=build/keiland-linux-clang
sh plan/tools/keiland-linux/elf-check.sh build/keiland-linux/stage && sh plan/tools/keiland-linux/makefile-sync.sh && sh plan/tools/keiland-linux/header-check.sh
cc -std=gnu17 -Wall -Wextra -Werror -o build/keiland-linux/stage/opt/keiland/bin/dmabuf-forge plan/tools/keiland-linux/dmabuf-forge.c -I. -Iuserland/desktop/keiland \
   -Lbuild/keiland-linux/lib -l:libwayland-client.so -l:libvulkan.so.1 -Wl,-rpath-link,build/keiland-linux/lib -Wl,-rpath,/opt/keiland/lib
G=plan/tools/keiland-linux/guest.sh
sh $G start && sh plan/tools/keiland-linux/install-guest.sh
sh $G ssh 'rm -f /tmp/wayland.log; openvt -c 7 -s -- sh -c "KEILAND_SEAT=direct /opt/keiland/bin/wayland --session --glass --log-frames --socket=/run/keiland-0 > /tmp/wayland.log 2>&1"'
sh $G ssh 'for i in $(seq 30); do grep -q "ZWL READY" /tmp/wayland.log && break; sleep 1; done'
sh $G ssh 'XDG_RUNTIME_DIR=/run WAYLAND_DISPLAY=keiland-0 timeout 120 /opt/keiland/bin/wltest --windowed --size=640x420 --frames=600 > /tmp/wltest.log 2>&1; echo "wltest exit=$?"'
sh $G ssh 'grep -c "ZWL IMPORT client" /tmp/wayland.log; grep -c "ZWL ACQUIRE_FENCE" /tmp/wayland.log; tail -3 /tmp/wltest.log'
sh $G ssh 'XDG_RUNTIME_DIR=/run WAYLAND_DISPLAY=keiland-0 nohup /opt/keiland/bin/wltest --windowed --size=640x420 --frames=3600 --delay-ms=30 > /dev/null 2>&1 &'
sleep 3; sh $G screenshot $PWD/build/keiland-linux/p007-wltest.png
sh $G ssh 'XDG_RUNTIME_DIR=/run WAYLAND_DISPLAY=keiland-0 nohup /opt/keiland/bin/mview --windowed --size=960x640 > /tmp/mview.log 2>&1 &'
sleep 8; sh $G screenshot $PWD/build/keiland-linux/p007-mview.png
sh $G ssh 'XDG_RUNTIME_DIR=/run WAYLAND_DISPLAY=keiland-0 timeout 30 /opt/keiland/bin/dmabuf-forge; echo "forge exit=$?"; grep -c IMPORT_ERROR /tmp/wayland.log; pidof wayland'
sh $G ssh 'P=$(pidof wayland); ls /proc/$P/fd | wc -l; for i in 1 2 3 4 5; do XDG_RUNTIME_DIR=/run WAYLAND_DISPLAY=keiland-0 timeout 60 /opt/keiland/bin/wltest --frames=60 >/dev/null 2>&1; done; ls /proc/$P/fd | wc -l'
```

`dmabuf-forge.c`: 我々の libvulkan-compat で小さな（64×64）dma-buf の image を作って export し（survey/vkexp.c の形）、我々の libwayland-client で `zwp_linux_dmabuf_v1` を bind し、
params に `add(fd, 0, 0, stride, 0, 0)` → `create_immed(64, 4096, ARGB8888, 0)`（高さを偽って dma-buf より大きくする）を送り、roundtrip で protocol の error（`out_of_bounds`）で
切られることを確かめて `dmabuf-forge: PASS`。

## 完了の条件

1. build（gcc・clang）、`elf-check: PASS`、`makefile-sync: PASS`、`header-check: PASS`。
2. guest: `wltest exit=0`、compositor の log に `ZWL IMPORT client` の行と `ZWL ACQUIRE_FENCE` の行（commit ごと）。screenshot に wltest の窓の絵（PNG をユーザーに見せる、`png-probe.py` で窓の中の色が wallpaper と違う）。
3. guest: `mview` の窓が出て model が描かれる（screenshot、PNG をユーザーに見せる）。
4. 偽の buffer: `dmabuf-forge: PASS`、compositor の log に `IMPORT_ERROR` が増え、`pidof wayland` が残る（動き続けている）。
5. 窓を 5 回開いて閉じた前後で compositor の fd の数が同じ（±2 以内）。
6. design §10 の V5 の結果（別の process の dma-buf の import）。
7. zedBSD の回帰（design §9.2。共通の file と `zedbsd/` の module を変えたので、[WS104 の commands.md](../../../ws104/commands.md) の §1・§4・§5・§6 の全部）。

## 結果

cleared（q532）。source `2d4abde1`（WIP）。Linuxのstandard zwp_linux_dmabuf_v1 v3 server / 1-plane validation / SCM_RIGHTSの所有 / Vulkan import / commitごとのimplicit acquire syncを実装。bind・object-free hookを既存OS境界へ追加、zedBSDは空実装。wltestとmviewの独立Linux buildとmodel data、test-only dmabuf-forgeを追加。gcc14.2 / clang19.1.7 exit0・warning0、15ELF / source-sync / 135headers PASS。formatter19・新moduleのstyle-check0、該当全文規約のmanual review。Linux guest: wltest600frame exit0・3import・600acquirefence、窓内部RGB(32,96,208)、mview model（25861vertex / 37000triangle / 13texture）描画。両PNGを目視・ユーザーに提示。forge out_of_bounds6 / IMPORT_ERROR / compositor継続 PASS、5窓fd20→20、SIGTERM error0 / cleanup_failed0、guest停止済み。V5の別process dma-buf importを確認。

zedBSD: disk-image exit0・自前warning0、OS境界C1〜C5 / V1（54source）、dedicated18 / decoder17 ×ordinary/sanitize、login PNG、forge拒否後の120frame / 3import、fence600（generation1全600、62秒）PASS。C1/C2/C9の元13件は10PASS・3FAILを保持。p076のresize416（期待200）は既存[BUG-125](../../../bugs/BUG-125.md)、p072の最小化直後PNGは[BUG-127](../../../bugs/BUG-127.md)へ未修正trackingとして移管。ユーザーは当チャットで「move/resizeは、Linux移植と関係ないバグの可能性があるので、いったんバグリストに記載するか、既存バグチケットに追記して、先に進みましょう。clear判定に進んでいいです。また、直せそうなら直してもいいですが、時間がかかりそうなら直さなくていいです。」と具体的なclear判断を許可。両件の長い追加調査は実施しない。修理・13/13PASSとは主張しない。cursor-ownerはtitle画像だけFAIL、同じsource・imageの単独1回で全条件PASS（title57 / desktop118 / body0 / body-again0）。[BUG-118](../../../bugs/BUG-118.md)に元と追試の証拠を追記、原因と発生率は未調査で、既存修正の無効化は未証明。

[証拠とSHA256manifest](evidence/SHA256SUMS)。未実施:実機GPU、非同期hardware wait / FOREIGN queue ownership（design既存V3/V8/V9の制限）、tracked2件の修正。その他のp007必須条件は確認済み。GitHub publication / closeは未実施、eventはoutboxに保持。WS105はincomplete、次はp008。

実装 commit: `2d4abde19fa2837bb9b7014d1bf931542d557c21`（WIP）。終了 UTC: 2026-10-01T09:45:04.063506+00:00。GitHub は未公開、Phase / WS event と intended close は outbox に保持。

## 開始前の設計補完（2026-10-01、Q1）

現行 `bind_global` は GPU factory の bind event を呼ぶ hook を持たない。D6 の v3 format / modifier 通知を実現するため、既存の OS 境界と同じ `int zwl_gpu_bind(struct zwl_object *)` を追加する。common は factory の bind 直後に呼び、Linux が event を送る。zedBSD は 0 を返す空実装。`zwl_gpu_object_free` の宣言も `zwl-gpu.h` に置く。技術上の欠落の補完で、product・受け入れ・依存・他 Phase は変えない。p007 の Queue 選定前に scope に反映。

## 実行 checkpoint（2026-10-01）

q532 / source `2d4abde1`（WIP）。Linux gcc14.2 / clang19.1.7 warning0、15ELF / source-sync / 135header PASS、new Linux GPU module / forge fixture の formatter19・style-check0・全文規約の manual review。guest wltest600frame exit0、3import / 600acquirefence。別processのimportされた窓のPNGは内部2点でRGB(32,96,208)、mviewの25861vertex / 37000triangle / 13texture model表示。forgeはparamsのout_of_bounds6を受信、compositor継続。5窓の開閉でfd20→20。SIGTERMはerror0 / cleanup_failed0、guest停止。Linux PNGはユーザーに表示済み。zedBSD必須回帰は直列実行中（未完了）。実機GPUの非同期wait / FOREIGN queue ownershipは既存designの制限を維持。

## Target 回帰の調査 checkpoint（2026-10-01）

q532 の C9 で p072 と p076 が FAIL。p072 は最小化ログあり、最初のPNGだけ青い窓とminimize位置のcursorが残り、復元とdesktop移動はPASS。p076 はleft resizeのendが416（期待200）、後続800も未到達。既存BUG-125の症状に近いが原因を断定しない。独立項目は続行、Phaseはin-progressを維持。未知調査上限30分で、元の失敗を保持し、同一imageの対象2試験を1回だけdiagnostic trace付きで再実行して切り分ける。production source / criteriaは変更しない。追加のretryや既知bugの修理へ拡張するときは結果と設計を再評価する。

### 2026-10-01 window操作のユーザー判断

ユーザー（当チャット）: 「move/resizeは、Linux移植と関係ないバグの可能性があるので、いったんバグリストに記載するか、既存バグチケットに追記して、先に進みましょう。clear判定に進んでいいです。また、直せそうなら直してもいいですが、時間がかかりそうなら直さなくていいです。」

p076はBUG-125へ追加、p072はBUG-127へ移管。両方のFAILは保持し、修正済みとはしない。準備した追加diagnosticは未実施。その他の必須確認と新しいcursor-ownerの失敗は別途確認する。全13件PASSとは記録しない。GitHub公開は保留。
