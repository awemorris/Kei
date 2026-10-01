<!-- awesome-plan project=zedbsd record=ws105 -->

# WS105: Keiland を Linux で動かす（`/opt/keiland`）

<!-- awesome-plan-current:start -->
Status: incomplete
Primary Milestone: MG006
Related Milestones: MG007
Objectives: O2
Parent: [Master](../master.md)
Queue: q523 finished
Resume point: p002 cleared（q524）。次は依存を満たす既存 Phase。
<!-- awesome-plan-current:end -->

## 目標

Keiland の compositor・library・主な app を、**Linux（Debian 13 で確かめる）の上で `/opt/keiland` に install して動かす**。

- `make keiland-linux` で build し、`make keiland-linux-install` で `/opt/keiland` に入れる。
- app は `/opt/keiland/lib/libvulkan.so.1`（libvulkan-compat）を通して system の Vulkan（Mesa）で描き、我々の compositor が Linux の KMS に出す。
- text console から root で起動でき、gdm から session として起動できる。
- Settings の network（WiFi）と音量が Linux の仕組み（wpa_supplicant・ALSA）で動く。

FreeBSD は範囲の外（[F-065](../future/F-065-keiland-portable.md) に残す）。

## 背景

2026-09-30 にユーザーが、Keiland を Linux・FreeBSD でも使う構成を決めた（F-065: 組み込み Linux と、GPL の無い FreeBSD のため。`/opt/keiland` に全てを入れ、
distribution の規則は無視する。Vulkan は我々の WSI を持つ libvulkan から system の libvulkan に chain する）。2026-10-01 にユーザーが「Linux移植を進めます」と言い、
Q1 と手順を検討して、次の「決定と理由」を確定した。zedBSD の上での準備（OS の部分を module に閉じる）は [WS104](../ws104/ws.md) が行う。

**仕組みの詳細は [design.md](design.md)（2026-10-01 の改訂 2: survey と design-reviewer の結果を入れた）。** 特に libvulkan-compat（§4）は、Vulkan の dispatch・ELF の名前の解決・Wayland の dma-buf の知識が要る難しい部分なので、
触る agent は §4 を全部読んでから始める。

## 決定と理由（2026-10-01 ユーザーとの検討）

この WS の全ての Phase は、次の決定の上に立つ。**決定を Phase の中で変えない。** 変える必要が分かったら、Phase を uncleared で止めて main（Q1）に報告する。

背景: 2026-09-30 にユーザーが Keiland（compositor・libwayland-client・libkeiland・app）を Linux・FreeBSD でも使う構成を決めた（[F-065](../future/F-065-keiland-portable.md)）。
動機は (1) 組み込み Linux で Keiland を使う、(2) GPL の無い FreeBSD で Keiland を使う。2026-10-01 にユーザーが「Linux移植を進めます」と言い、
次の決定を一つずつ検討して確定した。

| # | 決定 | 理由（検討の経緯） | 出典 |
| --- | --- | --- | --- |
| D1 | build は `make keiland-linux`、install は `make keiland-linux-install`（既定で `/opt/keiland` に入れる） | ユーザーの指定。distribution の規則（`/usr`・FHS・XDG）に合わせず、`/opt/keiland` の下に全てを閉じる（F-065 の決定 4）。Mesa などの driver は distribution の `/usr` の物を使う | 2026-10-01 ユーザー |
| D2 | package ごとに `Makefile.linux` を作り、zedBSD の `Makefile` とは**完全に別の build の設定**にする | ユーザーの指定。zedBSD の build（cross の toolchain・sysroot・image の規則、`ZEDBSD_USERLAND_PACKAGE`）を Linux に持ち込まない。両者の source の一覧のずれは script で検出する（p002） | 2026-10-01 ユーザー |
| D3 | `keiland.h` などの desktop の公開の header は、libc（`include/libc/`）から `userland/desktop/keiland/` へ移す。`vulkan/` は libc に残す | `keiland.h` は libc の物ではなく desktop の library の物。Linux の build は zedBSD の libc の header（`include/libc`）を読めない（glibc の header と衝突する）ので、desktop の header を libc から分ける必要がある。`vulkan/` は「OS の API」と見なす（ユーザー「zedBSDではこれがOSのAPIです」）。Linux では host の `/usr/include/vulkan` を使う。Wayland の header（`wayland-*.h`）は我々の libwayland-client が upstream と ABI が違う（core だけ、F-065 の決定 1）ため、host の物を使ってはいけないので一緒に移す（2026-10-01 ユーザー了承）。移すのは WS104 | 2026-10-01 ユーザー |
| D4 | `userland/desktop/libvulkan-compat/` を新しく作る。独自の WSI を実装し、system の libvulkan（後段）に chain する。**Linux の target でだけ build し、zedBSD の target では build しない** | ユーザーの指定。F-065 の決定 7（後段は dlopen し関数の pointer で呼ぶ、symbol の名前は変えない、後段の WSI は使わない）を実装する物。仕組みは [design.md](design.md) §4 | 2026-10-01 ユーザー、F-065 |
| D5 | 後段の WSI は使わない。後段は「描画できる Vulkan」でさえあればよい | 組み込みでは Mesa でないベンダーの libvulkan（Mali・PowerVR・Adreno）が多く、それらは Wayland の WSI を持たないか、別の版の libwayland を前提にする。`/usr/lib/libvulkan.so` を入口にすれば、中身が Khronos の loader でもベンダーの単体の libvulkan でも同じに扱える | 2026-09-30 ユーザー（F-065 の決定 7） |
| D6 | **Linux では client と compositor の間の buffer は Linux の標準の `zwp_linux_dmabuf_v1`、fence は implicit sync（dma-buf に sync_file を付ける `DMA_BUF_IOCTL_IMPORT_SYNC_FILE`・`EXPORT_SYNC_FILE`）。`keiland_gpu_buffer_v1` は zedBSD だけの protocol になる** | ユーザーの提案「keiland_gpu_buffer_v1を実装しないで、Linuxのdma-bufferを実装すればいいんじゃないですか」を Q1 が確かめて採った。利点: (1) libvulkan-compat を host の GNOME（mutter）・weston の上で、我々の compositor より先に試せる、(2) 我々の compositor が system の Mesa を使う普通の Linux の app（Firefox など）も表示できる、(3) implicit sync は Mesa の WSI と同じ方法で、どの compositor でも動き、compositor は poll するだけ（WS103 の形）。F-065 の「全 OS で独自の protocol 1 本」は改めた（2026-10-01 ユーザー了承）。explicit sync（`linux-drm-syncobj-v1`）は後で足せる。sync_file の ioctl は Linux 6.0 から。古い kernel では client が CPU で描画の完了を待ってから commit する予備の道を持つ | 2026-10-01 ユーザー |
| D7 | libvulkan-compat は zedBSD の `userland/desktop/libvulkan/` の source を共有しない（読んで手本にするのはよい） | zedBSD の libvulkan は Venus・i915 の driver そのもので、WSI も kernel の handle・`internal.h` の struct に深く結び付いている。共有の境界を作る費用が、新しく書く費用より大きい | 2026-10-01 Q1 の判断（委ねられた技術の範囲。ユーザーに示し、異議なし） |
| D8 | compositor の画面の出力: libvulkan-compat が **KMS を直接使って VK_KHR_display を実装**する（後段の VK_KHR_display は使わない）。DRM の fd は compositor の seat の backend が得て、規格の `VK_EXT_acquire_drm_display`（`vkGetDrmDisplayEXT`・`vkAcquireDrmDisplayEXT`）で渡す | ベンダーの libvulkan は VK_KHR_display を持たないことが多い。「後段の WSI は使わない」（D5）と一貫する。gdm の下では compositor は利用者として動き、DRM の device は logind からしか得られないので、fd を Vulkan に渡す規格の道が要る。F-065 の未決 2 の (a)（2026-10-01 ユーザー了承） | 2026-10-01 ユーザー |
| D9 | KMS は Linux の kernel の DRM の ioctl を直接使い、libdrm に link しない。DRM の header は linux-libc-dev の `<drm/*.h>`（MIT） | 依存を減らす（組み込みで libdrm が無い・古いことがある）。使う ioctl は 12 個ほどで小さい | 2026-10-01 Q1 の判断（委ねられた技術の範囲） |
| D10 | **後段を `RTLD_DEEPBIND` で開く（glibc の既定、`KEILAND_VULKAN_NO_DEEPBIND=1` で外せる）**。加えて、I の関数の後段の版は `dlsym` の pointer だけで呼び、同じ関数への再入を検出して `abort` する。我々の library は `-Wl,-Bsymbolic` で link する | F-065 の未決 1（2026-09-30 ユーザー「作業時に確かめる点として記録します」）。2026-10-01 の計画の初版は「DEEPBIND は opt-in」だったが、design-reviewer と survey の実験で、Debian の Khronos の loader（`-Bsymbolic` 無し）は自分の `vk*` の address を 269 個の relocation で取っていて、我々の library が先に載ると 215 個が我々の関数に結び付き、loader の `vkGetInstanceProcAddr` が我々の関数を返し、loader の中から我々の関数が実際に呼ばれることが分かった。`RTLD_DEEPBIND` で 0 になる（確かめ済み）ので既定にした（design §4.11）。外すのは、app が malloc などを preload で差し替える場合のため | 2026-10-01 Q1 の判断（委ねられた技術の範囲。実験の証拠による初版の改訂） |
| D11 | sessiond は Linux に移植しない。**gdm から `/opt/keiland/bin/wayland` が起動されればよい** | ユーザーの指定（2026-10-01 Q1「問題ないか」の確かめ: compositor は sessiond 無しで画面を取る道を既に持つ（`handoff.c`）。gdm の下では compositor が利用者として動くので、DRM と入力の device を logind の `TakeDevice` で得る seat の backend が要る。Log Out は compositor の終了（gdm が greeter に戻す）。session の中の Shut Down は zedBSD にも無い（greeter だけが持つ）ので、Linux では gdm の greeter に任せる（2026-10-01 の計画で Q1 が具体化）。gdm に見せる `/usr/share/wayland-sessions/keiland.desktop` は `/opt/keiland` の外の唯一の file） | 2026-10-01 ユーザー |
| D12 | logind とは D-Bus を自前の最小の実装で話す（libsystemd・libdbus に link しない） | libsystemd・libelogind は LGPL で、F-065 の「全て再実装する」に合わない。使う method は `GetSession`・`TakeControl`・`TakeDevice`・`ReleaseDevice`・`PauseDeviceComplete` と signal の `PauseDevice`・`ResumeDevice` だけ | 2026-10-01 Q1 の判断（ユーザーの「全て再実装」の方針、F-065 の決定 6 から） |
| D13 | 開発の最初は root で text console から起動する（`seat-direct`）。gdm（`seat-logind`）は後の Phase | 権限の仕組みを後に回し、画面・入力・buffer を先に確かめる。組み込みの Linux（logind が無い）でも `seat-direct` を使う | 2026-10-01 Q1 の提案（ユーザーに示し、異議なし） |
| D14 | libkeiland の OS の部分は、OS ごとの C の source に分ける（macro の block は非常に細かい所だけ）。仕組みが Linux と FreeBSD で同じ物（wpa_supplicant）は仕組みの名前の directory、OS に固有の物は OS の名前の directory | ユーザーの指定「思い切って分ける」。wpa_supplicant は BSD license で FreeBSD の base にも入っているので、`wpa/` として共有できる | 2026-10-01 ユーザー |
| D15 | Linux の WiFi は wpa_supplicant の制御 socket を直接話す。IP の取得（DHCP）は system（dhcpcd・NetworkManager 無しの構成では udhcpc など）に任せる。音は ALSA の kernel の interface（`/dev/snd/controlC*`）を直接使い、alsa-lib に link しない | 組み込みの Linux で最も広く使える。zedBSD の networkd・audiod の移植は zedBSD の kernel の ioctl に依存するので書き直しが大きい | 2026-10-01 Q1 の推奨（ユーザーに示し、異議なし。明示の決定ではない） |
| D16 | compositor の evdev は、Linux 用の source に分ける（backend の分離として）。key の code の定数（`KEY_*` など）だけは小さな内部の header 1 つの macro で切り替える | ユーザーの指定「evdevは念のため、Linuxはソースコードを分けましょう」。zedBSD の入力は Linux の evdev と同じ API だが、hotplug・権限（logind）・device の扱いが違う | 2026-10-01 ユーザー |
| D17 | compositor の `zwl_buffer_layout` は zedBSD の module の中だけで使い、Linux では使わない。OS の境界を「wire の値を decode する」から「client の buffer を VkImage と memory にする」に上げる | ユーザーの提案。Linux の buffer は modifier と複数の plane を持ち、zedBSD の 64 byte の記述と形が違う。共通の code が知るのは幅・高さ・VkFormat・VkImage・VkDeviceMemory だけにする。境界の引き上げは WS104 | 2026-10-01 ユーザー |
| D18 | WS を 2 つに分ける: **WS104**（zedBSD の上での OS の境界の整理、zedBSD の振る舞いは変えない）と **WS105**（Linux への移植）。FreeBSD は範囲の外（F-065 に残す） | 「WS は一つの具体的な到達目標」の規則。WS104 は zedBSD だけで閉じて確かめられる | 2026-10-01 ユーザー |
| D19 | Linux でも我々の libwayland-client の SONAME は `libwayland-client.so`（版の番号無し、zedBSD と同じ）。system の `libwayland-client.so.0` と名前を変える | 後段の ICD（Mesa）は system の `libwayland-client.so.0` に依存する。名前が違えば両方が載り、ICD の参照は先に載った我々の物に結び付き（実行はされない）、我々に無い symbol は system の物で解決されて load が失敗しない。2026-10-01 に Mesa 25.0.7 の 4 つの ICD が参照する `wl_*` が全て我々の物にあることを `nm` で確かめた（[design.md](design.md) §4.11） | 2026-10-01 Q1 の調査と判断 |
| D20 | libvulkan-compat の画面の出力は、最初は「複写の道」（後段の image → host の buffer → KMS の dumb buffer）。複写しない道は後で | 全ての後段（lavapipe のように DRM と関係の無い software の Vulkan、ベンダーの libvulkan）で動く。QEMU の試験の host は lavapipe だけ | 2026-10-01 Q1 の判断（委ねられた技術の範囲） |
| D21 | WS105 で Linux に持って行く app: compositor、Terminal・Files・Settings・Notes・Text Editor・Image Viewer・PDF Viewer・IME・kuidemo・mview と試験の app（vkdemo・wltest・wlshm）。browser・xserver・EGL/GLES は範囲の外 | browser（133 file、openssl）と xserver・EGL/GLES は依存が大きく、移植の仕組みを確かめるのに要らない | 2026-10-01 Q1 の判断（範囲の具体化） |
| D22 | Linux の compositor は、自分の frame の fence を fd にしない（`vkGetFenceStatus` の poll の道） | Linux の SYNC_FD の fence の export は fence を reset する（規格の副作用）ので、今の `zwl_compose_complete` の `vkWaitForFences` と合わない。OPAQUE_FD は後段によっては無い | 2026-10-01 Q1 の判断 |
| D23 | Linux の試験は host（libvulkan-compat の chain と WSI、試験用の Wayland server）と QEMU の Debian 13 の guest（compositor・app・gdm・WiFi・音）で行う。host の画面と入力の device は触らない | host は server（Matrox、3D 無し、Vulkan は lavapipe だけ）。guest なら root で DRM master を取り、gdm を入れてよい | 2026-10-01 Q1 の判断。survey: guest の Venus は host の render node を要るので使わない。guest の image は root 無しで作れる（mmdebstrap の unshare）。試験の QEMU は起動ごとに overlay の qcow2 で、共有の image を書かない（design §7.2） |
| D24 | Linux の build の flag に `-Wno-format-truncation` を入れる。gcc だけの他の警告（`-Wmaybe-uninitialized` の誤検出）は、共通の code に初期値を入れて直す | 2026-10-01 の host の試しの compile で、gcc 14 は表示の文字列の切れの警告を 10 箇所出した（clang 19 は 0）。文字列は意図して切っている。`-Wmaybe-uninitialized` は clang が option を知らないので flag で抑えられない（2 箇所、design §3.5） | 2026-10-01 Q1 の判断（委ねられた技術の範囲） |
| D25 | **Linux の guest の起動の確かめは、SSH が通ることと QMP の `screendump` の PNG で行う**（`plan/tools/boot-test.sh` は使わない）。serial の log は判定に使わない | `boot-test.sh` は zedBSD の image の UEFI の起動用。Linux の guest は QEMU の direct kernel boot なので、ホストの `127.0.0.1:2225` から guest の port 22 への転送で SSH を確認し、QMP で画面を見る。AGENTS.md と Guardrail に WS105 限定の例外を記録 | 2026-10-01 ユーザー「許可します」 |

## 達成基準

| # | 基準 | 確かめ（証拠の場所） |
| --- | --- | --- |
| L1 | `make keiland-linux`（gcc と clang）が warning 0 で通り、`make keiland-linux-install` が §2 の配置を作る。全ての ELF が `RUNPATH /opt/keiland/lib` を持つ | p002・p008 の `elf-check.sh` |
| L2 | libvulkan-compat が system の libvulkan に chain する: `vk-chain-test` が後段の lavapipe で計算でき、`vulkaninfo --summary` が我々の library で走り、後段から我々への名前の結び付きが無い | p003（host） |
| L3 | libvulkan-compat の Wayland の WSI: `wsi-probe-client` の frame が `dmabuf-probe` に正しい色で届き、implicit sync の fence を待てる | p004（host） |
| L4 | libvulkan-compat の画面の WSI: guest で `display-probe`・vkdemo の画面が QMP の screenshot に正しい色で出る | p005（guest） |
| L5 | compositor が guest の VT で root で起動し、wallpaper と system bar を出し、wl_shm と Vulkan の client（wltest）の窓を出し、pointer と key が届く | p006・p007（guest） |
| L6 | §2 の app が App Home から起動して窓を出す。Terminal に打った文字が出る | p008（guest） |
| L7 | gdm の session の一覧から Keiland を選ぶと利用者（root でない）の session が起動し、Log Out で gdm に戻る。VT を切り替えて戻ると画面が戻る | p009（guest） |
| L8 | Settings の network の頁が wpa_supplicant の scan の結果を出し、保存した鍵で接続できる。system bar と Settings の音量が ALSA の音量を変える | p010（guest） |
| L9 | zedBSD の振る舞いが変わらない（WS105 で共通の file を変えた所の回帰） | 各 Phase と p011 |

## 守ること（全 Phase）

- **決定（上の表）を Phase の中で変えない。** 変える必要が分かったら Phase を uncleared で止め、理由と要る判断を記録して main に報告する。
- 共通の source（zedBSD と Linux で同じ file）は、design.md §5.6 に挙げた変更だけを行う。他の変更が要るなら止めて main に報告する。
- 共通の source を変えた Phase は zedBSD の回帰（design.md §9.2）を流す。
- Linux の code は `<package>/linux/<役割>-linux.c`、Linux と FreeBSD で共有する仕組みは `<package>/<仕組み>/<役割>-<仕組み>.c`、libvulkan-compat は `userland/desktop/libvulkan-compat/`。
- macro の block（`#if defined(__linux__)`）は、非常に細かい所だけ（今は `wayland/zwl-evdev.h` の 1 つ）。増やすときは理由を Phase に記録する。
- 新しい code には [coding-style.md](../coding-style.md) の全文を適用する。注釈・名前の付け方は周りの zedBSD の code に合わせる。
- host の画面（`/dev/dri/card0`、Matrox の console）と host の入力の device を開かない。KMS・evdev の試験は guest で行う。
- host に package を入れるとき（sudo、AGENTS.md で許可）は、Phase の結果に何を入れたかを書く。host の `/opt/keiland` には install しない（`DESTDIR` の tree を使う）。
- 証拠は host・guest（QEMU）を分けて書く。やっていない確かめは「未実施」と書く。PNG はユーザーに見せる。
- 外部の source（wpa_supplicant の `wpa_ctrl.c`、Mesa、weston など）は読んで手本にしてよいが、複写しない（license を混ぜない）。
- **host での試験の command には全て `KEILAND_DRM_DEVICE=none` を付け、`WAYLAND_DISPLAY`・`DISPLAY` を外す**（host の画面の DRM の device を開かないため。design §4.9・§7.1）。
- 全ての試験の command に `timeout` で上限を付ける（固まった client で agent が止まらないように）。
- `plan/tools/keiland-linux/` の道具と他の WS の file は main が merge する（subagent は自分の worktree で作り、main に送る）。
- 試験の guest は `guest.sh` が起動ごとに作る overlay で動かす。共有の `build/keiland-linux/guest/guest.img` を消さない・書かない（AGENTS.md の共有の build の規則）。
- 実験の code の手本は [survey/](survey/README.md)（2026-10-01 の survey が host で通した物）。

## Phase

| Phase | 目的 | 状態 | 依存 |
| --- | --- | --- | --- |
| [ws105-p001](phase001/phase.md) | Linux の試験の guest（QEMU の Debian 13）と操作の道具 | cleared | なし |
| [ws105-p002](phase002/phase.md) | build の土台（`keiland-linux.mk`・top-level の goal・library の `Makefile.linux`・ELF・source の一覧・header の漏れの確かめ） | cleared | WS104 の p001・p003 |
| [ws105-p003](phase003/phase.md) | libvulkan-compat (1): 後段への chain（WSI 無し） | planned | p002 |
| [ws105-p004](phase004/phase.md) | libvulkan-compat (2): Wayland の WSI（`zwp_linux_dmabuf_v1`、implicit sync） | planned | p003 |
| [ws105-p005](phase005/phase.md) | libvulkan-compat (3): 画面の WSI（KMS、VK_KHR_display、VK_EXT_acquire_drm_display） | planned | p004、p001 |
| [ws105-p006](phase006/phase.md) | compositor の Linux の build と module (1): seat-direct・入力・session（wl_shm の client まで） | planned | p005、WS104 完了 |
| [ws105-p007](phase007/phase.md) | compositor の Linux の module (2): `zwp_linux_dmabuf_v1` の server と implicit sync（Vulkan の client） | planned | p006 |
| [ws105-p008](phase008/phase.md) | app の Linux の build と install の data（font・wallpaper・設定） | planned | p007 |
| [ws105-p009](phase009/phase.md) | gdm と logind（seat-logind・最小の D-Bus・pause と resume・`keiland.desktop`） | planned | p008 |
| [ws105-p010](phase010/phase.md) | libkeiland の Linux の backend（wpa_supplicant・Linux の interface・ALSA） | planned | p008 |
| [ws105-p011](phase011/phase.md) | 規約の全文の見直し、境界の確かめの拡張、回帰（Linux と zedBSD）、install の文書 | planned | p001〜p010 |

依存の図（矢印は「前提 → 後」）:

```
p001 ──────────────────────────────┐
WS104 p001・p003 → p002 → p003 → p004 → p005 → p006（WS104 の完了も前提）→ p007 → p008 ┬→ p009 ┐
                                                                                         └→ p010 ┴→ p011
```

## 実行の体制

- p001 は今すぐ、p002・p003・p004 は WS104 の p001・p003 の後に、WS104 の残りと並行して進めてよい（p002〜p004 は host だけで閉じる）。p006 以降は WS104 の完了の後。
- libvulkan-compat（p003〜p005）と compositor（p006〜p007）、logind（p009）は phase-runner（high）。app の build（p008）と libkeiland の backend（p010）は
  phase-runner（high）か phase-runner-mid。
- 各 Phase の agent は、始める前に「決定と理由」・design.md の §0〜§3 と、その Phase が名指す節を読む。

## 自律実行の承認（2026-10-01）

current user, 2026-10-01「ws105の完了をゴールにして、自走をお願いします。」。既存 WS105 p001〜p011 の範囲を依存順の 1 Phase Queue で実装・検証・記録する。commit は WIP、push / GitHub 公開は行わない。

### q523 / ws105-p001（2026-10-01T06:04:15.180248+00:00）

cleared。Debian 13 の base guest と操作の道具を作成。`guest.sh` は小さな sh 入口から Python の controller を呼ぶ（QMP JSON と座標変換を shell escaping 無しで扱う）。依存 package は既存 host にあり、host package 追加なし。

- `timeout 600 sh .../build-guest.sh`: exit 0。mmdebstrap 121.9975 秒、raw ext4 8 GiB。mesa-vulkan-drivers 25.0.7-2+deb13u1 / libvulkan1 1.4.309.0-1 / weston 14.0.2-1 / linux-image-amd64 6.12.107-1。guest kernel 6.12.107+deb13-amd64。
- `timeout 200 .../guest.sh start`: 180 秒以内に `guest: ready`。root と kei の loopback SSH 成功。kei の audio/video/input/kvm/render/netdev、`/run/user/1000` を確認。
- DRM card0 / ALSA controlC0 / event0〜5、Vulkan llvmpipe、mac80211_hwsim wlan0/wlan1、ALSA `'Master'` を確認。
- QMP PNG の login prompt を目視・ユーザーに提示。png-probe の size `1280 800`、2 点の色 `#000000`。key / click / type が QMP error 無し。
- run2 / port2226 の同時起動・SSH・停止 PASS。両 guest 停止後 overlay 無し、base image の size / mtime は一致。build の再実行は既存 image を保持。
- 追加の file 転送 / install 確認: 専用 stage の probe.txt を guest に install し、get 後 cmp 一致。guest 停止済み。host の `/opt` は変更していない。
- sh syntax、Python compile、`git diff --check` PASS。Master Tools 登録済み。共通 product code の変更無し、zedBSD 回帰対象無し。

証拠: `build/ws105-p001/`（build.log、start.log、verify.log、devices.txt、user.txt、image-before/after.txt、received.txt）。永続 PNG・版・試験 summary は `plan/history/ws105/q523/`。gdm variant の実行は p009、compositor / app の動作は後続 Phase。console / serial log は読んでいない。


[Phase の結果](phase001/phase.md)、[Queue history](../history/queue-q523.md)。WS105 の受け入れは残りの Phase の確認を要する。

### q524 / ws105-p002（2026-10-01T06:10:50.723980+00:00）

cleared。Linux の独立 build の土台と、8 package の `Makefile.linux`（7 shared library と install しない digest archive）を実装。仮 network / audio backend は公開 API の署名を保持し、service 不在を返す。DNS は実際の `/etc/resolv.conf` の dotted IPv4 を読む。

- gcc / clang の最終 build exit 0、`-Werror`・warning 0。host gcc 14.2.0 / clang 19.1.7、GNU Make 4.4.1。target toolchain 変更無し、host package 追加無し。
- install した 7 ELF の `RUNPATH [/opt/keiland/lib]` と SONAME / NEEDED を確認、`elf-check: PASS (7 ELF)`。digest archive は stage に無い。
- source token の比較 `makefile-sync: PASS`、system-inclusive dependency の確認 `header-check: PASS (57 sources)`。system の Wayland / EGL / GLES header 混入無し。
- `lib-smoke: PASS`（version 21、network record / unreachable state、audio unavailable、resolver reader）。
- `make keiland-linux-clean` exit 0。Linux の lib / obj / stage を消し、p001 の guest.img は保持。
- `timeout 600 make -j64 disk-image`: exit 0、warning 0。Linux Makefile は target build に include されず、zedBSD source / toolchain に変更無し。共通 C source を直していないため runtime 回帰対象無し。
- 新規 4 C file は clang-format19（ColumnLimit 0）後、定義引数・3 条件の行を全文規約に従い復元。style-check total 0、手動全文レビュー、sh syntax、`git diff --check` PASS。仮 backend の常に拒む API の最終 return は規定の errno を保持。

証拠: `build/ws105-p002/` の gcc/clang（初回・最終）log、install/clean/zedbsd log、verify.log。永続の試験 summary と source manifest は `plan/history/ws105/q524/`。libvulkan と compositor / app は後続 Phase。host の `/opt/keiland` に install していない。host 試験は `KEILAND_DRM_DEVICE=none`、Wayland/X 環境を外し、timeout 付きで実施。


[Phase の結果](phase002/phase.md)、[Queue history](../history/queue-q524.md)。WS105 の受け入れは残りの Phase の確認を要する。
