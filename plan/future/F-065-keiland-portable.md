# F-065: Keiland を Linux・FreeBSD で動かす構成

Future Work の詳細。実行の許可ではない。着手するときは新しい WS を立てる。

## 動機（2026-09-30 ユーザー）

- 組み込み Linux で Keiland を使いたい。
- FreeBSD で、GPL の code なしに Keiland を使いたい。

## 決めたこと（2026-09-30 ユーザー）

1. **libwayland-client には core の protocol だけを入れる。**
2. **xdg-shell をはじめ core 以外の protocol は全て libkeiland（旧 libzdesktop）に入れる。** app は protocol を直接話さず、libkeiland の抽象を使う。
3. **`zed-*-client-protocol.h` は内部に置いてよいが、app には見せない。** 見るのは libkeiland だけ。
   例外として、GPU の buffer と fence の stub（`zed-gpu-buffer-v1-client-protocol.h`）は libvulkan の WSI だけが使うので、libvulkan の内部に置く（Q1 の補い）。
4. **Linux・FreeBSD でも、我々の Wayland compositor・libwayland-client・libkeiland を使う。**
   - `userland/desktop/` 以下の全てを `/opt/keiland/` に install する。compat の libpng なども `/opt/keiland/` に入れる。
   - 既存のディストリビューションの規則は無視し、バニラの Linux の上に構築できる形にする。
   - Linux の標準がどうなっているかは、ほぼ無視してよい。
5. **Linux・FreeBSD の compositor は OS の機能を使ってよい。**
   - dma-buf は使う。epoll・kqueue などは後で考える。
   - 対応は、macro の block か、OS ごとの source の module の入れ替えで行う。
6. **互換の Qt6（WS096）・GTK4（WS097）を新規に書き、Kei・Linux・FreeBSD の全てで使う。**
   - XDG などは気にしなくてよい。libkeiland が抽象化する。
   - 既存の仕組みは気にせず、全て再実装する。
7. **Vulkan は、Linux・FreeBSD では system の libvulkan を後段に使う。**
   - Mesa などの driver はディストリビューションが `/usr` に入れる。
   - app は、我々の独自の WSI を持つ libvulkan（`/opt/keiland/`）を使い、そこから後段の `/usr/lib/libvulkan.so`（とその ICD）へ chain する。
   - 後段の WSI は使わない。
   - 後段は `dlopen` し、`dlsym` で得た `vkGetInstanceProcAddr` から関数の pointer で呼ぶ。symbol の名前は変えない（2026-09-30 ユーザー
     「シンボルを変える必要はないです。バックエンドのlibvulkanは動的ロードしてシンボルをポインタでロードすればいいだけです。」）。
   - 理由: 組み込みで一般的な、Mesa でない libvulkan も使いたい。ベンダーの Vulkan（Mali・PowerVR・Adreno など）は、
     Khronos の loader と ICD の形ではなく、単体の `libvulkan.so` のことが多い。`/usr/lib/libvulkan.so` を入口にすれば、
     中身が Khronos の loader でもベンダーの単体の libvulkan でも同じに扱える。

## 7 から出ること（2026-09-30 Q1）

- 後段は Wayland を話さない（WSI を使わないため）。後段の library（Mesa の ICD、ベンダーの blob）が libwayland-client に link していて、
  その `wl_*` の参照が先に載った我々の library に結び付いても、実行されないので害は無い。我々に無い symbol は後ろの system の library で解決され、
  load も失敗しない。このため我々の libwayland-client は upstream と ABI 互換である必要がない（1 の「core だけ」が通る）。
- ~~client と compositor の間は、全 OS で我々の独自の protocol 1 本にでき、linux-dmabuf・syncobj は要らない。運ぶ中身だけが OS で変わる。~~
  **2026-10-01 改訂（ユーザー了承、[WS105](../ws105/ws.md) の D6）: Linux では Linux の標準の `zwp_linux_dmabuf_v1` と implicit sync（dma-buf に sync_file を付ける）を使い、
  `keiland_gpu_buffer_v1` は zedBSD だけの protocol にする。** 理由: libvulkan-compat を他の compositor の上で先に試せる、我々の compositor が普通の Linux の app も表示できる、
  implicit sync はどの compositor でも動く。下の表の Linux の行の「sync_file」は、protocol で送るのでなく dma-buf に付けて渡す。

  | OS | buffer | fence |
  | --- | --- | --- |
  | zedBSD | kernel handle の fd と記述（kernel と照合できる） | 世代付きの fence の fd |
  | Linux・FreeBSD | dma-buf の fd と fourcc・modifier・plane の offset と stride | sync_file |

- 我々の EGL・GLES（WS068）は我々の libvulkan の上に載るので、後段の GL は要らない。
- zedBSD の libvulkan も「前段（WSI、OS 共通）」と「後段（zedBSD は Venus と i915、Linux・FreeBSD は system の libvulkan）」に分ける形になる。

## Linux への着手（2026-10-01）

ユーザー「Linux移植を進めます」。**Linux の分は [WS104](../ws104/ws.md)（zedBSD の上での OS の境界の整理）と [WS105](../ws105/ws.md)（Linux への移植）に promote した。**
決定と理由は WS105 の ws.md の「決定と理由」（D1〜D25）、仕組みは [WS105 の design.md](../ws105/design.md)。FreeBSD はこの file に残る（deferred）。
下の未決の行き先: 1 → WS105 の D10 と p003 の確かめ、2 → (a)（WS105 の D8）、3 → WS105 の design §4.6・§10 の V3、4 → WS105 の D15、5・6 → まだ未決（WS105 の範囲の外）。

## 未決（着手のときに決める）

1. **後段の chain の細部と、同じ process の中の名前の確かめ。**（symbol の名前は変えない。決めたこと 7）
   - 前段が後段を dlopen し、`vkGetInstanceProcAddr` から後段の関数を得る形の細部（instance・device の dispatch の包み方）。
   - 確かめ 1: 後段が自分の公開の `vk*` の address を内部で使う場合（例 `vkGetInstanceProcAddr` が自分の `vkCreateDevice` を返す）、
     それが先に載った我々の同名の関数に解決されると前段と後段が循環する。Khronos の loader は `-Bsymbolic` で防いでいると Q1 は記憶しているが未確認。
     ベンダーの単体の libvulkan は不明。起きたら `RTLD_DEEPBIND` などで対処する。
     ELF の既定では同じ module の中は優先されない（symbol の interposition）。2026-09-30 Q1 の host（Debian、glibc）の試験: 後段の中から自分の同名の関数を呼ぶと、
     gcc の既定（`-O0`・`-O2`）では先に載った前段の物が呼ばれ、gcc `-Wl,-Bsymbolic` と clang `-O2` の既定では後段の物が呼ばれた。
     同じ module が勝つのは `-Bsymbolic`、hidden・protected の visibility、compiler が割り込み無しと見なした場合（clang の既定、gcc の
     `-fno-semantic-interposition`）。作業のときに、使う後段ごとに確かめる（ユーザー「作業時に確かめる点として記録します」）。
   - 確かめ 2: 後段が実際に呼ぶ library（Mesa は zlib を shader の cache、expat を driconf に使う）を我々も同じ symbol の名前で持つと、我々の物が呼ばれる。
     我々の compat の library が ABI まで互換なら問題ない。
2. **compositor の画面の出力（VK_KHR_display）も WSI の一部である。** 次のどちらにするか。
   - (a) 我々の libvulkan が KMS を直接使って実装する（「後段の WSI は使わない」に一貫する）。
   - (b) Mesa の VK_KHR_display と `VK_EXT_acquire_drm_display` を例外として通す。

   どちらでも、DRM master を得る仕組み（logind の無いバニラ Linux なら seatd か自前）と、greeter と session の間の画面の受け渡しが要る。
3. **後段に要る export の拡張。** `VK_EXT_external_memory_dma_buf`、`VK_EXT_image_drm_format_modifier`、SYNC_FD の export
   （`VK_KHR_external_semaphore_fd`・`VK_KHR_external_fence_fd`）。Mesa の anv・radv は持っているはずだが未確認。ベンダーの libvulkan は
   揃わないことがあり（modifier が無いなど）、足りないときの予備の道（linear だけ、copy など）が要るかもしれない。
4. **libkeiland の OS の backend。** `audio.c`・`network.c`・`network-link.c` は zedBSD の audiod・networkd と話していると思われる（未確認）。
   Linux・FreeBSD の backend をどうするか。
5. **app と libkeiland の移行。** 今の Keiland の app が xdg-shell などを直接話している所を、libkeiland の抽象へ移す作業の範囲。
6. **`wl_proxy_add_dispatcher` など、我々の libwayland の独自の関数の扱い。** core だけにするとき、libkeiland が使う公開の契約として残すかを決める。

## 関係

- [WS103](../ws103/ws.md): compositor の GPU の直の ioctl を無くし、buffer・fence の受け側を backend の境界の後ろに置く。その境界がこの構成の Linux・FreeBSD の backend の入口になる。
- 2026-09-30 の対話（Q1 の説明）: libwayland-client に Keiland の protocol の stub がある事、OPAQUE_FD と dma-buf の違い、`set_acquire_fence` が libvulkan の WSI の中で送られる事。


## Linuxの合意範囲の完了（2026-10-01）

[WS104](../ws104/ws.md) A1〜A6と[WS105](../ws105/ws.md) L1〜L9/最終conformanceを確認してcompleted。独立build/install、Vulkan backend/WSI、compositor、主なapp、gdm/logind、WiFi/WPA/ALSAをhostとown Debian13 QEMU guestで検証。[最終証拠](../history/ws105/q538/evidence/SHA256SUMS)。BUG-125/127はユーザー許可で未修正tracking、実機やMG006全体の完了とはしない。

Linux分のpromotionは結果を保持して終了。FreeBSDはdeferredのまま、互換Qt6/GTK4・EGL/GLES/browser・ARM/musl/実機・PCM・explicit syncなどdesign§8は本WSの未達作業として増やさない。再考triggerはユーザーがFreeBSDまたは当該追加scopeへの着手を指定したとき。新しいWS/有限Queueの選定・承認が要る。


## FreeBSD 分の promotion（2026-10-01 レビュー後）

ユーザーが Linux版Keiland の FreeBSD15 移植を指定。FreeBSD 分を [WS109](../ws109/ws.md) へ promote（planning、Queue無し）。
共通描画を再利用、audio/network/WiFi に FreeBSD backend。Linux 固有の dma-buf/sync ioctl、seat/VT/input、loader と license は先に点検する。
上の Linux・FreeBSD の buffer/fence 表は当初の設計意図。FreeBSD15 の実 ABI/能力が同じと確認済みではない（WS109 p001）。
Linux の completed と既知 bug の扱いを保持。互換 Qt/GTK、EGL/GLES、ARM/musl/PCM 等の未指定 design§8 は deferred のまま。
以前の FreeBSD deferred は当時の判断として保持し、この日付の promotion を現在の行き先とする。
[レビューの出典](../reviews/2026-10-01-review.md)。implementation は有限 Queue の選定/承認後。
