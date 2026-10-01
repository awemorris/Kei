# WS105 survey の実験の code（参照用）

2026-10-01 に Q1 の survey（subagent）が host（Debian 13、Mesa 25.0.7 の lavapipe、Khronos の loader 1.4.309）で [design.md](../../../../ws105/design.md) §4 の前提を確かめるために書いた実験の code。
**product の code ではない**（build の規則に入っていない。coding-style.md にも合わせていない）。p003・p004 の実装と試験の道具（`plan/tools/keiland-linux/`）の出発点・手本として読む。
license: zedBSD と同じ Zlib（Q1 の subagent が書いた物で、外部の code の複写ではない）。

| file | 何を確かめたか | 結果（2026-10-01） |
| --- | --- | --- |
| `vkexp.c` | lavapipe の DRM の modifier の一覧、modifier の image の作成と dma-buf の export、`vkGetImageSubresourceLayout(MEMORY_PLANE_0)`、mmap、別の device・別の process での import と画素の照合 | modifier は LINEAR（0）だけ。export・import・画素は一致。export の fd は CLOEXEC でない。import の fd は成功で lavapipe が閉じる。import の大きさは requirements の size（`lseek` は page に丸めた値） |
| `vksync.c` | SYNC_FD の semaphore・fence の export・import、`DMA_BUF_IOCTL_IMPORT_SYNC_FILE`・`EXPORT_SYNC_FILE`、fence の export の reset | 全て動く。lavapipe は CPU で完了まで待つので、export した sync_file は常に済み。`vkGetFenceFdKHR(SYNC_FD)` は fence を reset する。OPAQUE_FD の fence・semaphore は無い |
| `compat.c`・`gen-forward.sh`・`vkprotos.sh`・`exports.map` | 後段を `dlopen` で開いて dlsym の pointer で呼ぶ最小の forwarder（`libvulkan.so.1`） | instance・device が作れる。後段（loader）の `vkGetInstanceProcAddr` は instance の名前に**我々の**関数を返す（下）。`vkprotos.sh` は `vulkan_core.h` の prototype を抜き出す（1.0=137、1.1=28、1.2=13、1.3=37、1.4=19） |
| `chain.c`・`gipa.c` | forwarder を通した chain と、後段の GIPA の答え | 上 |
| `dmabuf-probe.c` | 試験用の Wayland server（host の libwayland-server、`wl_compositor` v4 + `zwp_linux_dmabuf_v1` v3、EXPORT_SYNC_FILE と poll と mmap、`PROBE` の行） | `wsi-client.c` の 3 frame が正しい色で届いた |
| `wsi-client.c` | system の libwayland-client と lavapipe で、dma-buf の image を作り `create_immed` で送る client | 上 |
| `build-guest.sh` | Linux の試験の guest の image を作る（mmdebstrap の `--mode=unshare`、sudo 無し、`mkfs.ext4 -d rootfs.tar`）。p001 の `build-guest.sh` の出発点 | 減らした package の組で 60 秒。KVM で起動し 9 秒で SSH、`/dev/dri/card0`・`/dev/snd/controlC0`・`mac80211_hwsim` を確かめた |
| `dbusprobe.py` | system bus への最小の D-Bus の client（NUL の byte、`AUTH EXTERNAL`、`NEGOTIATE_UNIX_FD`、`Hello`、method の call の marshal） | host の system bus で `GetSessionByPID` などが通った。p009 の `dbus-linux.c` の手本 |
| `asnd.c` | ALSA の control の ioctl の番号と struct の大きさ | `id` 64・`list` 80・`info` 272・`value` 1224・`event` 72 byte（amd64） |

build の例（host）:

```
X=$(pkg-config --variable=pkgdatadir wayland-protocols)/stable/linux-dmabuf/linux-dmabuf-v1.xml
SC=$(pkg-config --variable=wayland_scanner wayland-scanner)
mkdir -p /tmp/s5/gen
$SC server-header $X /tmp/s5/gen/linux-dmabuf-v1-server-protocol.h
$SC client-header $X /tmp/s5/gen/linux-dmabuf-v1-client-protocol.h
$SC private-code  $X /tmp/s5/gen/linux-dmabuf-v1-protocol.c
cc -std=c11 -Wall -Wextra -Werror -O2 -I/tmp/s5/gen $(pkg-config --cflags wayland-server) -o /tmp/s5/dmabuf-probe plan/ws105/survey/dmabuf-probe.c /tmp/s5/gen/linux-dmabuf-v1-protocol.c $(pkg-config --libs wayland-server)
sh plan/ws105/survey/vkprotos.sh /usr/include/vulkan/vulkan_core.h > /tmp/s5/core.tsv
```

**後段の名前の結び付き（重要、design §4.11）**: 我々の library を DT_NEEDED で載せると、Debian の loader の `vk*` の 215 個の参照（`R_X86_64_GLOB_DAT`、address を取るだけ。
`JUMP_SLOT`（名前で呼ぶ）は 0 個）が我々の export に結び付く。そのため loader の `vkGetInstanceProcAddr` は、instance の名前（`vkCreateInstance`・`vkCreateDevice`・
`vkGetPhysicalDeviceProperties`・`vkCmdDraw` など）に**我々の関数**を返し、`vkGetDeviceProcAddr` は `vkGetDeviceProcAddr`・`vkDestroyDevice`・`vkGetDeviceQueue` に我々の関数を返す。
`RTLD_DEEPBIND` で開けば 0 になる。


2026-10-01: WS105完了時に実験原文をarchiveへ移動。実行済みのproduction/reusable testではない。旧command pathとsourceの原文は歴史として保持、現在の道具は[README](../../../../tools/keiland-linux/README.md)。
