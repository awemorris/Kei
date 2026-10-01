<!-- awesome-plan project=zedbsd record=ws105 -->

# WS105: Keiland を Linux で動かす（`/opt/keiland`）

<!-- awesome-plan-current:start -->
Status: incomplete
Primary Milestone: MG006
Related Milestones: MG007
Objectives: O2
Parent: [Master](../master.md)
Queue: q536 finished
Resume point: p009 cleared（q536）。次は依存を満たす既存 Phase。
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
| [ws105-p003](phase003/phase.md) | libvulkan-compat (1): 後段への chain（WSI 無し） | cleared | p002 |
| [ws105-p004](phase004/phase.md) | libvulkan-compat (2): Wayland の WSI（`zwp_linux_dmabuf_v1`、implicit sync） | cleared | p003 |
| [ws105-p005](phase005/phase.md) | libvulkan-compat (3): 画面の WSI（KMS、VK_KHR_display、VK_EXT_acquire_drm_display） | cleared | p004、p001 |
| [ws105-p006](phase006/phase.md) | compositor の Linux の build と module (1): seat-direct・入力・session（wl_shm の client まで） | cleared | p005、WS104 完了 |
| [ws105-p007](phase007/phase.md) | compositor の Linux の module (2): `zwp_linux_dmabuf_v1` の server と implicit sync（Vulkan の client） | cleared | p006 |
| [ws105-p008](phase008/phase.md) | app の Linux の build と install の data（font・wallpaper・設定） | cleared | p007 |
| [ws105-p009](phase009/phase.md) | gdm と logind（seat-logind・最小の D-Bus・pause と resume・`keiland.desktop`） | cleared | p008、p005修復 |
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

### q525 / ws105-p003（2026-10-01T06:35:02.871528+00:00）

cleared。Linux 専用 libvulkan-compat を実装。後段への F 222 関数、I 12 関数、禁止する WSI N 51 関数を maintained TSV から生成。dispatchable handle を包まず、後段の dlsym の trampoline を使い、instance / physical-device / device / queue の ownership を保持する。mutex・pthread_once・pthread の thread-local record で publication / 再入 / lifetime を扱う。

- gcc / clang の最終 build exit 0、warning 0。8 ELF の RUNPATH / SONAME / NEEDED が PASS。libvulkan.so.1 の NEEDED は glibc の libc.so.6 だけ（libdl / pthread は glibc 2.34 以降 libc に統合）。初版の __thread の dynamic TLS による ld-linux 直接依存を pthread key に替えて、計画の依存条件を満たした。
- `vk-chain-test: PASS`: staged library の dladdr、Vulkan 1.0 instance、llvmpipe device / driver、1 MiB の fill → copy → fence、262144 word 全一致、GetProcAddr と禁止した X11 surface 名の NULL。
- 我々の library で `vulkaninfo --summary` が llvmpipe を表示。空 XDG_RUNTIME_DIR、Wayland/X を外し、host の DRM は none。
- `interpose-check: PASS`: 既定の backend → compat vk binding 0、NO_DEEPBIND opt-out の command も PASS（V1・V2 verified）。同じ SONAME の別 backend を同 process で利用できた。
- 自分自身の backend 指定と /nonexistent は両方、no backend libvulkan の診断で exit 1、timeout 124 ではない。明示した backend 選択は失敗を既定候補で隠さず、その指定を authoritative とする。
- fake backend の PLT 再入は指定の診断と exit 134。初回 fake は gcc が直接の再帰を local alias にしたため SIGSEGV、試験の extern assembler alias で PLT call を確認して直した。production の再入検出を既定のまま検証した。
- `elf-check: PASS (8 ELF)`、`makefile-sync: PASS`、`header-check: PASS (61 sources)`。system loader が export する WSI 名と TSV を照合し、未分類名 0。
- 新規 source と generated forward.inc の style-check total 0。clang-format19 後、定義引数と3条件を復元、全文を手動レビュー。sh syntax / git diff --check PASS。
- `timeout 600 make -j64 disk-image` exit 0、warning 0。zedBSD の source / toolchain の変更無し、共通 C source の runtime 回帰対象無し。host package 追加無し。

設計上の実装の補い: Layer / version の enumeration は、Phase 本文が要求する「後段が無い場合」の fallback を作るため I の表にも入れた（当初の I 10 個に2個追加）。外部の約束・依存・受け入れは変更無し。
証拠は `build/ws105-p003/`、永続の summary / manifest は `plan/history/ws105/q525/`。WSI の実装はまだ無く、Linux compositor / app / gdm / network / audio は後続。実機 GPU、古い backend の全ての組合せ、musl は未実施。push / GitHub publication は未実施。


[Phase の結果](phase003/phase.md)、[Queue history](../history/queue-q525.md)。WS105 の受け入れは残りの Phase の確認を要する。

### p004 の検証手順（q526 選定前）

全文規約 §12 に合わせ、CPU fallback の試験は production の専用環境変数を作らず、試験 shim の IMPORT_SYNC_FILE / ENOTTY から既定の道を通す。D6・L3 と frame / pixel / fence の受け入れ、他の Phase の依存は同じ。[p004](phase004/phase.md) に理由と手順を記録。

### q526 / ws105-p004（2026-10-01T07:22:11.469836+00:00）

uncleared。- **uncleared**: 4 種の client/server がともに exit 0、各 90 frame。FIFO・resize・MAILBOX の全画素/サイズ一致。fallback の画素/サイズは一致したが、承認済み `fences=0` は不成立。
- host kernel `6.12.101+deb13-amd64`、lavapipe。全 run の raw fences=1、driver/timeline=`stub`、status=1、waited_ms=0。既に完了した writer と空の reservation をこの観測だけでは区別できない。fences≥1 だけによる通常経路の受け入れも根拠が不足する。
- Linux 6.12 の一次資料: [dma-buf.c](https://raw.githubusercontent.com/torvalds/linux/v6.12/drivers/dma-buf/dma-buf.c) の dma_buf_export_sync_file は空の reservation に dma_fence_get_stub を補う。[dma-fence.c](https://raw.githubusercontent.com/torvalds/linux/v6.12/drivers/dma-buf/dma-fence.c) の stub は既に signal 済み。
- gcc の Linux WSI build warning 0。4 run の [観測](evidence/) を保存。clang/全規約/chain 回帰はこの試行では未実施。既存コードの partial resource cleanup と通常 acquire の初期 queue 記録を次試行で見直す。
- 再開条件: 試験だけの ioctl observer で通常 IMPORT_SYNC_FILE の成功/flags=WRITE、fallback ENOTTY 1 回・以後成功 import 0 回、CPU fence 待ち・90 frame 全画素を実測する。raw fence/timeline は加工せず残す。D6 の production 同期方式、L3 の画素/安全性、他 Phase の出力は変更しない。main の委任済み技術判断で検証の観測方法を改訂し、新 Queue snapshot に残す。
- valgrind は host に無く未実施。実機 GPU の非同期 fence 待ちは未実施、lavapipe のみ。push/GitHub 公開なし。


[Phase の結果](phase004/phase.md)、[Queue history](../history/queue-q526.md)。WS105 の受け入れは残りの Phase の確認を要する。

### p004 再設計 / p011 文書参照（2026-10-01）

q526 uncleared の観測問題を kernel 一次資料と照合し、[p004](phase004/phase.md) の検証を IMPORT_SYNC_FILE 成功/ENOTTY・CPU wait と実画素に改訂。raw kernel fence 数を加工せず記録する。D6 / L3 / 他 Phase の実装出力は同じ。[p011](phase011/phase.md) は禁止された未実装の試験環境変数の文書化を除いた。自走の委任済み技術判断として次 Queue に正確な snapshot を保持する。

### q527 / ws105-p004（2026-10-01T07:35:44.278649+00:00）

cleared。- **cleared**。q526 の旧条件が不成立だった履歴を保持し、kernel の stub に依存しない改訂検証を実施した。product API / D6 の implicit sync と CPU fallback は同じ。
- gcc 14.2.0 / clang 19.1.7 の最終 source build warning 0。`elf-check: PASS`（8 ELF）、`makefile-sync: PASS`、`header-check: PASS`（64 source）。`git diff --check` PASS。libvulkan-compat 全 C/header、変更した試験 C と generated forward.inc の style-check 合計0、ANSI 宣言・public/static順・callback storage寿命・fd ownership・error unwindを全文規約で照合。
- `timeout 120 bash plan/tools/keiland-linux/wsi-check.sh`: FIFO/fallback/resize/MAILBOX は各90 frame、client/server exit0。全360 frame の実画素がN%3の赤/緑/青と一致。resizeは320×240→400×300→320×240、MAILBOXも90frame。
- 通常3 run: IMPORT_SYNC_FILE flags=WRITE(2) がそれぞれ90回成功。private waitは各180回（acquire/reuse）、全て成功。fallback: ENOTTY(25)は最初の1回のみ、以後import試行無し。private waitは270回、CPU-before-commitが全90frameに加わり全て成功。observerは試験専用、backend DEEPBINDとproduction引数/戻り値を維持。
- raw SYNC_IOC_FILE_INFO は全runでfences=1、driver/timeline=stub、status=1、waited_ms=0。値を加工して0にせず保存。この値だけをimplicit sync成功の根拠にしない。V3: lavapipeの実際のcreated modifier=0x0、single plane、stride=1280/1600、offset=0を確認。V9はkernel importの成功と実画素で検証。
- `vk-chain-test: PASS`: staged SONAME、surface/wayland拡張有り、XCB/Xlib無し、未enableのWayland procedure=NULL、API1.0、llvmpipe、1MiB fill/copy一致。`interpose-check: PASS`、default backend-to-compat bindings=0、NO_DEEPBIND optoutもPASS。
- `make -j4 disk-image` exit0、warning0。Linux固有library/testだけの変更のためzedBSD runtime回帰はp011の全体回帰で実施。host package追加・target toolchain変更・host /opt install無し。
- swapchain destroy時にGPU資源を先に退役し、未releaseのWayland callback storageだけをsurfaceで保持する。deviceが先に破棄されてもcallback dataが残る。初回acquireより前のapplication queue retrievalを必須にしない。
- [raw evidence](../../history/ws105/q527/evidence/) と [ELF manifest](../../history/ws105/q527/manifest.sha256)を保存。valgrindはhostに無く未実施。実機GPUの非同期待ちは未実施、host lavapipeのみ。GitHub publication/remote closeはdeferred、outboxで保持。commit WIP、push無し。


[Phase の結果](phase004/phase.md)、[Queue history](../history/queue-q527.md)。WS105 の受け入れは残りの Phase の確認を要する。

### q528 / ws105-p005（2026-10-01T08:13:06.450181+00:00）

cleared。cleared。VK_KHR_display・direct-mode・DRM acquisition の O 10 entry、KMS inquiry/dup master/saved CRTC/double dumb FIFO copy、Linux vkdemo を実装した。

- gcc 14.2 / clang 19.1.7: final build exit0、warning0。elf-check PASS（10 ELF、probe を含む）、makefile-sync PASS、header-check PASS（70 sources）。clang-format19＋定義の改行、scoped style-check（実装・probe）0件。新規 KMS source と変更部分を全文規約で手動点検した。最終全 WS conformance は p011。
- host は DRM=none、display 環境無し。vk-chain-test PASS（API1.0・llvmpipe・1MiB全word一致）、interpose PASS（backend→compat binding0・opt-out）、Wayland FIFO/fallback/resize/MAILBOX 各90 frame、360色/extent/import/private wait PASS。
- guest kernel6.12.107+deb13-amd64、Mesa25.0.7、lavapipe。seat fd duplicate（元fdをclose）とdirectの2経路で Virtual-1 / 1280×800 / 74994mHz。赤・緑・青全6PNG各4点一致、probe exit0/PASS。赤→緑で live oldSwapchain を更新・破棄し、master所有権の継承を確認した。
- vkdemo --time-ms=1000 --hold=10: 320×240、中心 #20c5b0、描画PNGを表示、VKDEMO DONE frames=1。各経路の終了後と最終chvt1はconsole文字のPNGを確認・表示。
- 途中chvt1→5秒→chvt7: direct/root はmasterを失わず、3色を完走してPASS。実際のlogind revoke/OUT_OF_DATE はp009で確認する。ioctlのEACCES/EPERM→OUT_OF_DATEと100ms上限はsourceで確認。
- 完了後 guest stop、overlay廃棄。host package追加0、target toolchain/common zedBSD source変更0。実機GPU・物理monitorのcustom mode・Valgrindは未実施。問合せ/display/mode handleはprocess-lifetime、実機hotplugの動的再列挙は範囲外。
- [ログ・PNG・sha256 manifest](../../history/ws105/q528/evidence/)。不具合残件なし。次はp006（root compositor・wl_shm・入力・VT）、前提WS104とp005を確認。


[Phase の結果](phase005/phase.md)、[Queue history](../history/queue-q528.md)。WS105 の受け入れは残りの Phase の確認を要する。

### q529 / ws105-p006（2026-10-01T08:25:21.947014+00:00）

uncleared。uncleared（q529-i01）。Linux compositor/direct seat/evdev/VT/handoff/GPUなしglobalと、独立Makefile・fonts・wlshmを実装した。gcc/clang exit0 warning0、12ELF / source-sync / 126header / scoped style PASS。

- guest: READY、Virtual-1 1280×800。wallpaper/bar (100,10)=#ebf9ec / (100,400)=#c5dde6。wl_shm center (640,400) background (224,235,248)→(0,0,255)、約6000frame表示。cursor差は(200,200)と(900,600)の付近。App HomeとEsc、Super+TabのWiseviewを確認。
- 手順のmeta単独でHomeという記述は実装と違った。既存c5-transitions.shと同じlauncher click (23,17) に訂正、keyboardはEsc/Super+Tabで確認。受け入れ範囲は同じ。
- WiseviewをEscで閉じる連続描画で `ZWL VULKAN_ERROR operation=submit result=-1000001004`、`ZWL FAILED site=compose_draw errno=5`、EXIT frames=6051 error5 cleanup_failed1。masterはseatのroot fdで保持。KMSが100ms総期限をOUT_OF_DATEとする実装を確認、設計は各pollの上限100msであり総期限の意図ではなかった。タイミングからpoll deadlineが原因と推定、次のp005修正Queueでbounded遅延試験により確かめる。
- Log Outは上記でcompositorが先に終了したため未達。この終了をLog Out成功とは数えない。独立したSIGTERM終了とseat未指定のSSH tty sessionではREADY→EXIT error0、chvt1文字とconsole key 'kei' PNGを確認。
- zedBSD disk-image exit0、C1〜C5 boundary、v1、dedicated/decode host、boot login PNG確認。C1/C2/C9＋forge/fenceの承認済み回帰processは進行中（exec session52085、outputs build/ws105-p006）。既存承認の検証だけを継続して証拠を保存し、再開Queueでterminal結果を確認する。これらを今PASSと扱わない。
- Linux guest stop済み、overlay廃棄。sourceはWIPに保存。mainの委任された技術判断でp005を再開し、poll1回≤100ms＋有限の総期限のKMS待機に直す。p005の修正と再検証後、同じp006を再開してLog Out・SIGTERM・keyboard・zedBSD回帰の全条件を確認する。WS105の受け入れは変更しない。


[Phase の結果](phase006/phase.md)、[Queue history](../history/queue-q529.md)。WS105 の受け入れは残りの Phase の確認を要する。

### q529後のKMS待機の再設計

p006はuncleared、p005のclearanceをinvalidated。100ms総期限の誤りをkms.cで修正し、test-only遅延observerで再検証後にp006を再開する。[p005](phase005/phase.md)・[origin p006](phase006/phase.md)。WS105の目的・受け入れは同じ。既存の実装とq528/q529 historyを保持する。

### q530 / ws105-p005（2026-10-01T08:31:05.223429+00:00）

cleared。cleared（q530-i01）。KMS completionのpollを各100ms以下、総期限5秒にした。単発timeout/EINTRはretry、総期限未完了はSURFACE_LOST、EACCES/EPERMとrevoked fdはOUT_OF_DATEを維持した。productionの試験用switchは無し。

- 旧staged implementationにtest-only flip-delay.soで1回250ms遅延＋poll result0を与え、赤→緑の後にFAIL result=-1000001004を再現した。修正後のseat fd/direct両経路は同じinjection (`requested_ms=100 delayed_ms=250 result=0`) 後に青を表示しPASS/exit0。全6色PNG各4点一致、oldSwapchain破棄後も表示、console復元、guest stop。
- gcc14.2/clang19.1.7 build exit0 warning0、12ELF/source-sync/126header PASS（p006の既存Linuxprogramを含む）。style-check0、clangformat19、変更KMS節の全文規約を点検。全WSの最終規約はp011で実施。
- host DRM=none: chain1MiB/PASS、interposebindings0/optoutPASS、Wayland FIFO/fallback/resize/MAILBOX360frameの画素/extent/import/privatewait PASS。
- q528のvkdemoとrootVT切替の結果は保持。今回変更はその単発SETCRTCの経路に影響しない。logind/revoked fdの実動作は既存p009で確認。実機GPU/物理monitor/Valgrind未実施。
- [新旧ログ・PNG・manifest](../../history/ws105/q530/evidence/)。p005のclearanceを復旧し、p006の同じ受け入れを次のQueueで再開する。q529のunclearedは履歴として保持。


[Phase の結果](phase005/phase.md)、[Queue history](../history/queue-q530.md)。WS105 の受け入れは残りの Phase の確認を要する。

### p007 開始前の OS 境界補完（2026-10-01、Q1）

`bind_global` の GPU factory の event hook が現行 code にないため、`zwl_gpu_bind` を Linux / zedBSD の module に追加する設計を補完。common は hook を呼ぶだけ、zedBSD は空実装。既存 D6 の通知を実現し、受け入れと依存を維持。[p007](phase007/phase.md)、[design §5.2 / §5.6](design.md)。

### q531 / ws105-p006（2026-10-01T08:50:34.308158+00:00）

cleared。cleared（q531-i01）。Linux の seat-direct・入力・session・wl_shm の compositor を実装・検証した。p006 source は `80eea509`、依存 KMS repair は `753b45a0`（いずれも WIP）。q529 の uncleared は元の履歴として保持する。

- gcc14.2 / clang19.1.7 build exit0 warning0、12ELF / source-sync / 126header PASS。Linux の OS module は `linux/` に分離し、compositor は表示を Vulkan だけで扱う。evdev は monotonic clock、KD/keyboard mode は個別保存・復元。全文規約と style-check を点検した（最終 WS conformance は p011）。
- Linux guest: desktop / wlshm / Home / Esc / Wiseview と pointer 2 地点の PNG を確認。bar、wallpaper、青い window の画素、cursor の差分を確認。launcher は実際の top-left click を使い、Super+Tab は Wiseview（検証手順の補正、product code は不変）。
- Home の Log Out: `ZWL SESSION logout`、`ZWL EXIT frames=1081 error=0 cleanup_failed=0`。コンソールで `kei` が入力される PNG を確認。再起動は KEILAND_SEAT 未指定・XDG_RUNTIME_DIR=/run・--socket 未指定で `/run/wayland-keiland` に READY、SIGTERM は `frames=1 error=0 cleanup_failed=0`。PNG はユーザーに表示、guest は停止し overlay を破棄。
- zedBSD: disk-image exit0 / 自前 source warning0、boot-test PASS（login PNG 確認・表示）、OS boundary C1〜C5 / v1 54source PASS、dedicated18・decode17（通常 / sanitize とも）PASS、C1/C2/C9 13/13 PASS（C2 14/14、Wiseview・復元・Files の PNG を目視）、forge-guest PASS（偽buffer拒否後も正しいclient120frame）、fence-guest PASS（600frame、600fence、全generation1、62秒）。guest停止。
- [ログ・PNG・SHA256 manifest](../../history/ws105/q531/evidence/)。Target 回帰は q529 で開始した同一 source の直列実行を q531 へ引き継いだ。q530 は Linux KMS module のみの修正で、target source / toolchain はその間変更していない。
- 実機 GPU / 物理モニタ未実施。Linux GPU client は次 p007、logind は p009、app/data は p008、network/audio は p010 の既存範囲。GitHub 未公開、outbox に証拠・event と intended close を保持、push なし。


[Phase の結果](phase006/phase.md)、[Queue history](../history/queue-q531.md)。WS105 の受け入れは残りの Phase の確認を要する。

### q532 / p007 checkpoint・p008 の検証手順補正（2026-10-01）

p007 の Linux 検証は PASS、source `2d4abde1`（WIP）。zedBSD の回帰は実行中なので Phase は in-progress。p008 の App Home 手順は実装に合う launcher click に補正（受け入れと依存は不変）。[p007](phase007/phase.md)、[p008](phase008/phase.md)。

### p008 選定前の install data 具体化（2026-10-01、Q1）

既存 Home の apps.conf 入口で Settings・kuidemo を含む D21 の app を提供する。ユーザー wallpaper は WS035 の git 外の決定を維持し、既存元を install、cache のない環境は既存 Aurora の fallback / 明示元指定。Image Viewer は既存対応形式の PNG fixture で検証（PPM reader 追加なし）。受け入れと依存は不変。[p008](phase008/phase.md)、[design §3.3](design.md)。

### 2026-10-01 window操作のユーザー判断

ユーザー（当チャット）: 「move/resizeは、Linux移植と関係ないバグの可能性があるので、いったんバグリストに記載するか、既存バグチケットに追記して、先に進みましょう。clear判定に進んでいいです。また、直せそうなら直してもいいですが、時間がかかりそうなら直さなくていいです。」

p076はBUG-125へ追加、p072はBUG-127へ移管。両方のFAILは保持し、修正済みとはしない。準備した追加diagnosticは未実施。その他の必須確認と新しいcursor-ownerの失敗は別途確認する。全13件PASSとは記録しない。GitHub公開は保留。

### q532 / ws105-p007（2026-10-01T09:45:04.063506+00:00）

cleared。cleared（q532）。source `2d4abde1`（WIP）。Linuxのstandard zwp_linux_dmabuf_v1 v3 server / 1-plane validation / SCM_RIGHTSの所有 / Vulkan import / commitごとのimplicit acquire syncを実装。bind・object-free hookを既存OS境界へ追加、zedBSDは空実装。wltestとmviewの独立Linux buildとmodel data、test-only dmabuf-forgeを追加。gcc14.2 / clang19.1.7 exit0・warning0、15ELF / source-sync / 135headers PASS。formatter19・新moduleのstyle-check0、該当全文規約のmanual review。Linux guest: wltest600frame exit0・3import・600acquirefence、窓内部RGB(32,96,208)、mview model（25861vertex / 37000triangle / 13texture）描画。両PNGを目視・ユーザーに提示。forge out_of_bounds6 / IMPORT_ERROR / compositor継続 PASS、5窓fd20→20、SIGTERM error0 / cleanup_failed0、guest停止済み。V5の別process dma-buf importを確認。

zedBSD: disk-image exit0・自前warning0、OS境界C1〜C5 / V1（54source）、dedicated18 / decoder17 ×ordinary/sanitize、login PNG、forge拒否後の120frame / 3import、fence600（generation1全600、62秒）PASS。C1/C2/C9の元13件は10PASS・3FAILを保持。p076のresize416（期待200）は既存[BUG-125](../../bugs/BUG-125.md)、p072の最小化直後PNGは[BUG-127](../../bugs/BUG-127.md)へ未修正trackingとして移管。ユーザーは当チャットで「move/resizeは、Linux移植と関係ないバグの可能性があるので、いったんバグリストに記載するか、既存バグチケットに追記して、先に進みましょう。clear判定に進んでいいです。また、直せそうなら直してもいいですが、時間がかかりそうなら直さなくていいです。」と具体的なclear判断を許可。両件の長い追加調査は実施しない。修理・13/13PASSとは主張しない。cursor-ownerはtitle画像だけFAIL、同じsource・imageの単独1回で全条件PASS（title57 / desktop118 / body0 / body-again0）。[BUG-118](../../bugs/BUG-118.md)に元と追試の証拠を追記、原因と発生率は未調査で、既存修正の無効化は未証明。

[証拠とSHA256manifest](../../history/ws105/q532/evidence/SHA256SUMS)。未実施:実機GPU、非同期hardware wait / FOREIGN queue ownership（design既存V3/V8/V9の制限）、tracked2件の修正。その他のp007必須条件は確認済み。GitHub publication / closeは未実施、eventはoutboxに保持。WS105はincomplete、次はp008。

[Phase の結果](phase007/phase.md)、[Queue history](../history/queue-q532.md)。WS105 の受け入れは残りの Phase の確認を要する。

## Terminal 起動の bounded 補完（2026-10-01、Q1）

App HomeのTerminal childがstatus139、直接起動も同じ。source/objdumpでmain_start→main_menu_stateが最初のmain_tab_newより先、main_screenはNULLのままselection/rangeを読むと確認。OS分岐の問題ではない。D21のTerminal起動・入力という既存受け入れに必要な普通の技術修正として、common terminal/main.c:main_menu_stateを「screenが無ければ選択なし」にする。起動順、menu/tabs/shellの所有、product、依存、受け入れは不変。広いTerminal改修はしない。Linuxの修正前139→修正後Home起動・10秒生存・echo入力、zedBSDのTerminal起動/文字/終了と必須回帰で検証。ユーザーのWS105完了まで自走指示の委任を適用し、move/resizeのbug移管判断とは分ける。

## Linux 検証 checkpoint（2026-10-01、q533）

source `ba46edf8` + bounded Terminal修正 `7dd3ad9e`（WIP）。gcc14.2 / clang19.1.7 warning0、26ELF / source-sync / 329header PASS、4 common sourceのstyle-check0、changed scopeの全文manual review。App Homeから9appを起動して10秒生存。Terminal echo keiland-linux-ok、Filesのdirectory3item、Settingsのページ移動、Text Editor入力、Image Viewerのwallpapers内PNG、PDF Viewerのwriter-plain.pdf（2ページ）、Widget Demo、Model viewerを実画像で確認。IME Alt+Space→kanji→漢字→確定/直接入力への切替PASS。各appの代表PNGを目視・ユーザーに提示。Notesのkeyboard文字入力だけ未実施（既存の手書き専用appでその操作を提供していない）。Notes起動/終了はPASS。

全appはtitlebarのcloseで終了、Text Editorは保存確認のDon't Saveを追加で選び、最終psで0（desktop FilesとIMEのみ）。全Home childの正常終了status0を確認。初回Terminalだけstatus139を保持（menuがscreenを作る前にNULLを読むstartup問題、guardでLinuxPASSに修正、zedBSDの確認は残る）。compositor SIGTERM2310frame error0 / cleanup_failed0、guest停止済み。辞書archiveとdictionaryのSHA256検証、既定wallpaperはzedBSDとbyte一致、5gradient生成/install。ホストのpackage追加0。共通の3fileのtarget path文字列はWS104 q521と3/3同一。

[Linux証拠](../../history/ws105/q533/evidence/SHA256SUMS)。zedBSD必須回帰は直列実行中、p007で移管したp072/p076は今回各PASSだが修正とはしない。p008はin-progressのまま。

## 開始前の実装接続の具体化（2026-10-01、Q1）

実際のp006 sourceではseat-linux.hの共通Linux helperをseat-direct-linux.cが全て定義し、mainはOS dispatchより前にevdevを読む。p009のlogindを接続するため、root helperの実体をzwl_linux_direct_*へrenameし、既存zwl_linux_*のseat選択dispatchをos-linux.cへ置く。seat-linux.hに両backendのprivate関数を宣言し、Makefile.linuxにD-Bus / logind sourceを追加する。compositorの公開API、共通OS境界、rootのdeviceの扱いは不変。新しいproduct・方針の決定ではなく、既存D11/D12のOS内接続を具体化する。

PauseDeviceの処理をevdevの読みより先に行うようmainのOS poll doneを移す。inputのpaused fdはcommon入力recordのfd=-1でpollから外し、実fdとTakeDeviceの所有はlogindのrecordに残す。古いpoll snapshotからfd=-1を読まないguardをmainに追加。ResumeDeviceは新fdを同じ入力recordへ戻し、旧fdを閉じる。goneと通常closeではReleaseDevice/所有を一度ずつ返す。DRM pauseはos_pausedを立ててoutputを閉じ、windowed=0でresume後に既存zwl_schedule→enter_window_modeを通して再生成する。queued signalはsocketのreventsが0でもdispatchする。

D-Busは64KiB message / 16FD / bounded signal queue、readableになってからでもMSG_DONTWAITで読む。recvmsgは固定header16bytesとそのmessageの残りだけを読み、次のmessageのfdを混ぜない。同期callは5秒、signalを保持し、壊れたmessageとoverflowでは所有fdを閉じて失敗。device fdはCLOEXEC。Linux新moduleは全文規約で作り、実gdmのuid/VT pause/force/resume/LogOutとrootdirectの回帰で確認。

### q533 / ws105-p008（2026-10-01T10:24:11.027596+00:00）

cleared。Linux app の build/install、Home の9app・IME、data を検証し p008 cleared。source `ba46edf8` + Terminal guard `7dd3ad9e`（WIP）。gcc14.2 / clang19.1.7 warning0、26ELF（24 production + 2 test fixture）、source-sync / 329 header PASS、changed common source 4file style-check0。Homeの全appは起動10秒生存、Terminal echo、Files directory、Settingsページ、Text Editor文字、Image Viewer画像、PDF Viewer 2ページ、kuidemo / mviewを確認。IME Alt+Space→kanji→漢字→確定→直接入力PASS。Notesは起動/終了PASS、keyboard文字入力のみ理由つき未実施（既存手書き専用UI）。全appのcloseでchild status0、最後のpsはdesktop FilesとIMEのみ。compositor SIGTERM2310frame error0 / cleanup_failed0、guest停止。辞書SHA256、5gradient、既定wallpaperはtargetとbyte一致、host package追加0。

zedBSD: disk-image warning0、OS boundary / V1（54source）、host dedicated18 / decoder17 ordinary+sanitize、boot login PNG、C1/C2/C9 13/13、forge拒否→3import、fence600すべてgeneration1 PASS。3commonfileのtarget path stringsはWS104 q521と3/3一致。全criteria imageはTerminal guard前（startupを試験しない）；その後final sourceを含むforge imageでTerminalの10秒生存、echo keiland-zedbsd-ok表示、timeout終了（TAB count0）、compositor継続を別に確認、BUG-128 resolved。代表PNGを目視・ユーザーに提示、実機未実施。p072 / p076今回はPASSだが修理とはしない。BUG-125 / BUG-127のtrackingとq532の元のFAILは保持。

証拠: [q533 manifest](../../history/ws105/q533/evidence/SHA256SUMS)、original `build/ws105-p008/`。全guest停止。GitHub未公開、bug disposition / Phase / WS eventはoutboxで保持、pushなし。次はp009 logind / gdm。

[Phase の結果](phase008/phase.md)、[Queue history](../history/queue-q533.md)。WS105 の受け入れは残りの Phase の確認を要する。

### q534 / ws105-p009（2026-10-01T10:42:07.310406+00:00）

uncleared。uncleared（q534-i01、未達の前提で停止）。sourceはWIP commitに保存。D-Bus AUTH EXTERNAL / UNIX_FD / Hello、bounded frame / 16FD / signal queue / method deadline5s、logind control/device/pause/resume、backend dispatch、common os_paused / old poll snapshot guard、gdm session install / guest setupを実装。gcc14.2 / clang19.1.7 build warning0、26ELF、source-sync、331header、4Linux Cのstyle-check0 PASS。

gdm専用guestを作成し、psはuser kei / XDG session、logには正しいescaped session pathを確認。しかし最初のdisplay acquireでresult=-3、frames0 error5となり自動loginが繰り返されたためgdmを停止。試験用LD_PRELOAD observerでlibraryのDRM_IOCTL_SET_MASTER（0x641e）がerrno13/EACCESと確認した。Linux drm_auth.c の drm_master_check_perm はlogindが開いた共有fdのSET_MASTERに現在processの権限を要求する。現在のkms.cは渡されたmaster fdにも無条件SET_MASTERを行い、root専用のp005試験では発見できなかった。未知の前提を発見したためこのattemptを終了し、修正をp005で選定する。p009のVT / app / LogOut / rootdirect / target回帰は未実施、clear免除なし。

証拠 original build/ws105-p009/{gcc2,clang,elf,sources,headers,session-check,drm-observe,stop-loop}.log / session-first.png（画面が出ない、受け入れFAIL）。gdm停止、Linux専用guestは修正確認のため稼働、hostにpackage追加0、host gdmは触れていない。試験observerはguest overlayだけで本番には入れない。再開条件: p005がlogindから渡されたmaster fdをSET_MASTER不要で扱えることを検証、root direct / CRTC復元の回帰PASS。その後p009同じ基準で再試行。GitHub未公開、pushなし。


[Phase の結果](phase009/phase.md)、[Queue history](../history/queue-q534.md)。WS105 の受け入れは残りの Phase の確認を要する。

### q534 後の KMS fd ownership 修復

q534 の gdm 起動で kms.c の無条件 SET_MASTER が logind 共有fdに errno13 を返した。p005の「seatが渡すmaster fd」という出力を未達と判断してcurrent clearanceをinvalidated / uncleared（q528/q530は当時の結果を保持）。Q1のWS105完了までの委任された技術判断で、caller supplied fdはAUTH_MAGIC magic0の非破壊probe（current masterだけEINVAL）で検証し、SET_MASTER/DROP_MASTERはlibrary自身のdirect master取得だけに限定する。libraryはsupplied fdのdupとCRTCの復元だけを所有し、masterの制御はseat/logindへ返す。compositorにDRM ioctlは追加しない。影響はkms.cとprivate compat_displayのownership field。product/API/受け入れ目標は不変。

再検証: gcc/clang warning0・ELF/source/header・host chain/interpose/Wayland。guest rootのseat fd/direct 3色、oldSwapchain・console復元。q534のLinux logind sourceをfixture contextとして一般user gdm起動が表示できることを確認（p009のapp/VT/LogOut受け入れは次attempt）。偽の非masterfdはacquireで拒否し、callerfdはcloseされない。p009はp008と修復p005を依存として、同じ全基準で再実行。p006〜p008のroot経路で検証した受け入れは維持し、p011でfinalsourceを再確認。

[origin p009](phase009/phase.md) / [changed p005](phase005/phase.md)。依存 graphの追加edgeはp005修復→p009 retry。

### q535 / ws105-p005（2026-10-01T10:49:48.408582+00:00）

cleared。cleared（q535-i01、p005のlogind fd ownership修復）。sourceは今回のKMS WIP commit。caller supplied master fdはAUTH_MAGIC magic0でcurrent-masterを確かめ、SET_MASTER不要。非masterは拒否。libraryのdirect取得だけmaster_owned=1とし、release時のDROP_MASTERもその経路だけ。borrowed seat fdのdupはcloseし、callerのfd/master権限は保つ。compositorへのDRM ioctl追加なし、public API不変。

- gcc14.2 / clang19.1.7 warning0、26ELF（24production+2fixture）/ source-sync / 331header PASS。clangformat19 / kms.cと新fixture style-check0、changed KMS ownership scopeを全文規約でreview。host DRMnone chain1MiB・interpose0 / optout・Wayland FIFO/fallback/resize/MAILBOX360frame PASS。
- guestの既存display-probeはseat fd/directの両経路で赤・緑・青全6PNGの4点一致、oldSwapchainの置換と破棄後も表示、各exit0/PASS。gdm停止直後のtty1にはgettyが無くBIOS画面を復元したので、そのPNGも残し、getty@tty1を起動後に両経路を再確認、Linux login prompt復元PNGを目視。guest-only新fixture seat-fdはnonmaster拒否、両callerfileがlive、release後のcaller master維持PASS。
- source3400a098のlogind seatを既存contextとしてgdm自動loginのuser keiでKeiland wallpaper / system barを表示、PNGを目視・提示。q534のSET_MASTER EACCES / frames0から回復した。LD_PRELOAD observerはstaged installで除去、production経路。p009のapp/VT/LogOutはこれから同じ条件で再検証する。
- [証拠](../../history/ws105/q535/evidence/SHA256SUMS)。元build/ws105-p005-logind/とbuild/ws105-p009/kms-*を保持。実機未実施、host package追加0、toolchain / target source変更0。guestはp009再開のため稼働、gdm停止。q528/q530当時の結果とq534失敗は保持。GitHub未公開、pushなし。


[Phase の結果](phase005/phase.md)、[Queue history](../history/queue-q535.md)。WS105 の受け入れは残りの Phase の確認を要する。

### q536 / ws105-p009（2026-10-01T11:23:22.269889+00:00）

cleared。cleared（q536-i01）。p005のlogind fd修復5012d324を前提に、p009 source3400a098と入力lease補正8b0c6ee4で全7基準を検証。gcc14.2 / clang19.1.7 build warning0、26ELF（24本体+2fixture）、makefile-sync、331source header-check、変更Linux C / DBus fixture style-check0 PASS。公開keiland/OS API不変、compositor DRM ioctl0、toolchain変更なし。

Linux QEMU Debian13 gdm専用guest: 自動loginはuser kei、XDG_SESSION_TYPE=wayland / ID171 / RUNTIME/run/user/1000、escaped logind session path、wallpaper / systembarをPNGで確認。HomeからTerminal起動とecho入力PASS。SwitchTo・chvtの両方で同一PID7648を維持、DRMと4evdevの5leaseすべてのPauseDevice/ResumeDevice、復帰画面 / pointer / echo switch-ok・chvt-ok PASS。kernel revokeがD-Bus通知より先に届く入力ENODEVはleaseを保持しEAGAINとする補正、後のResumeDevice fdへ交換を実測。途中の補正前検証は原ログに保持。

V11の実観測: このsystemd257の両コマンドはtype=force（SwitchToをcooperative pauseと捏造しない）。DRM revocation後のCRTC restoreにPermissionDeniedが記録されるが、quiesce/output閉鎖→resume/swapchain再生成はPASS。pause-type ACK分岐はsource確認のみ、実guest通知未実施。外部deviceの実機hotplug / systemd再起動は未実施。

自動loginを切り、QMPでkei/passwordを入力→Keiland userkei PID8493→Home LogOut→gdm greeterへ戻るPNG PASS。最初のpassword入力はUI遷移待ち不足で拒否、focus後同じpasswordで成功（元PNGとlogを保持）。gdm guest停止、overlay破棄。baseguestも最新版stageをinstallし、root KEILAND_SEAT=direct / --session --glassを起動、wallpaper / systembar、Home Terminal echo keiland-direct-ok PASS。SIGTERM frames62/error0/cleanup_failed0、console復元、guest停止。

D-Bus実production clientの独立wire fixture: byte分割、call中2signal queue、各frameのSCM_RIGHTS分離/CLOEXEC/payload、返信serial、fd所有移管、missing right / 64KiB超過 / partialheader+right EOF / ancillary17fd truncationの拒否と全fd回収 PASS。同じ5caseのASan/UBSanも全PASS。

zedBSD: disk-image warning0、OS boundary / GPU V1、dedicated-host / gpu-zedbsd-host ordinary+sanitize、boot-test loginPNG PASS。C1/C2/C9は全13PASS。forge-guest PASS（3imports / 120frame）、fence-guest PASS（600fences / 600frame / generation1）。全target-regression PASS。

証拠 [q536 manifest](../../history/ws105/q536/evidence/SHA256SUMS)。PNG目視済み、代表画面は当チャットに表示。host追加package0、host画面/入力を使用せず、host /opt installなし。QEMUと実機を区別、実機未実施。BUG-125 / BUG-127は未修正tracking、前のq532 FAILを維持。GitHub publication / close はoutbox pending、pushなし。次は既存p010 network/ALSA、その後p011全文規約とWS最終受け入れ。


[Phase の結果](phase009/phase.md)、[Queue history](../history/queue-q536.md)。WS105 の受け入れは残りの Phase の確認を要する。
