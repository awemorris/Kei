<!-- awesome-plan project=zedbsd record=ws105 -->

# WS105: Keiland を Linux で動かす（`/opt/keiland`）

<!-- awesome-plan-current:start -->
Status: completed
Primary Milestone: MG006
Related Milestones: MG007
Objectives: O2
Parent: [Master](../master.md)
Queue: q538 finished
Resume point: L1〜L9・最終source conformance verified（2026-10-01）。実装の残りなし。GitHub publication / Issue close / Project projection は deferred。
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

## 達成基準と最終判定（2026-10-01、Q1）

全PhaseのStatusだけで判定せず、WS自身のL1〜L9を実際の最終出力で照合した。最終検証source `c7e8a35a`（WIP）、WS105 base `39a0941c^`、[source SHA256](../history/ws105/q538/source-SHA256SUMS)。

| # | 基準 | 最終証拠 / 判定 |
| --- | --- | --- |
| L1 | gcc/clangでwarning0、install配置と全ELFのRUNPATH /opt/keiland/lib | PASS、clean双方・24production ELF各・331header/source-sync。hostはDESTDIR、guestの/optにinstall。増分buildに隠れたlibrary/program object名衝突をp011で補完 |
| L2 | system Vulkan chain、lavapipe compute、vulkaninfo、backend名前の結び付き | PASS、1MiB/262144word一致、binding0、NO_DEEPBIND opt-out、vulkaninfo |
| L3 | Wayland WSIの画素とimplicit fence | PASS、FIFO/fallback/resize/MAILBOX各90frame、actual import observer/CPU wait。nonasync lavapipeのraw fenceだけで同期有無を判定しない |
| L4 | guestのKMS display-probe/vkdemoの実画面 | PASS、seat fd/direct6色×4点、oldSwapchain、CRTC console復元、vkdemo。caller fd/master維持とnonmaster拒否 |
| L5 | root VT desktop、wallpaper/bar、wl_shm/Vulkan窓、pointer/key | PASS、own guestで実画面。最終Vulkan窓600frame+5×20、18imports/700fences、RGB、fd29→29、forge拒否後継続 |
| L6 | §2の主なappがHomeから起動、Terminal入力 | PASS、Files/Terminal/Settings/Notes/Textedit/Image Viewer/PDF Viewer/kuidemo/mview9app、Terminal echo、Textedit日本語変換/確定、画像/PDF2page/model表示。Notesは既存の手書きUI（keyboard文字入力未実装） |
| L7 | gdm chooserのKeiland、nonroot login、LogOut、VT復帰 | PASS、manual userkei /Wayland/runtime1000、SwitchTo/chvt samePID/all5leases、10pause/resume、復帰echo、HomeLogOut→greeter。実通知force、cooperative ACKはsource確認のみ |
| L8 | SettingsのWiFi scan/保存した鍵で接続、bar/SettingsからALSA変更 | PASS、real hwsim/hostapd/wpa、UIでkey保存/COMPLETED/Connected、HDA Masterのactual readback。Settings74・bar20（整数丸め） |
| L9 | zedBSD共通変更の回帰 | build/boot/GPU/forge/fence/glass/Settings/audio/volume PASS。C1/C2/C9元11PASS/2resize FAILを保存、具体的ユーザー許可でBUG-125へ未修正tracking、clearを阻害しない。BUG-127も未修正tracking。全13PASS/修理済みとは主張しない |

[全文規約の照合](../history/ws105/q538/conformance.md)、[最終証拠manifest](../history/ws105/q538/evidence/SHA256SUMS)、[q538](../history/queue-q538.md)。各Phaseの前提出力も確認、WS104はcompleted。実機のデモを含むMG006/fg010全体を完了と判定していない。

## 守ることと実行の承認

D1〜D25、[Guardrail](../guardrail.md)、[coding-style.md](../coding-style.md)全文、designのOS/API/module境界を保持。current userの2026-10-01「ws105の完了をゴールにして、自走をお願いします。」により既存p001〜p011を1Phase Queueで実行。Q1/main単独、N=0。commitは全てWIP、push/GitHub公開はしない。人間のWS095 IME source・locked toolchain・Noctは変更しない。host package追加0、host /opt installなし、host画面/入力を使わない。host VulkanはDRM_DEVICE=none、Wayland/X環境を外しtimeoutを付けた。image buildsは直列、own fresh guest/overlayで確認。console/serial/kernel logを判定に使わない。

## Phaseと履歴

| Phase | 目的 | Status | 依存 | 最終結果 |
| --- | --- | --- | --- | --- |
| [ws105-p001](../history/ws105/q523/phase.md) | Linux guestと操作道具 | cleared | なし | [q523](../history/queue-q523.md) |
| [ws105-p002](../history/ws105/q524/phase.md) | 独立build/ELF/source/header | cleared | WS104 p001/p003 | [q524](../history/queue-q524.md) |
| [ws105-p003](../history/ws105/q525/phase.md) | Vulkan backend chain | cleared | p002 | [q525](../history/queue-q525.md) |
| [ws105-p004](../history/ws105/q527/phase.md) | Wayland dma-buf WSI | cleared | p003 | [q527](../history/queue-q527.md) |
| [ws105-p005](../history/ws105/q535/phase.md) | KMS display WSI /seat fd ownership | cleared | p004/p001 | [q535](../history/queue-q535.md) |
| [ws105-p006](../history/ws105/q531/phase.md) | root seat/input/session | cleared | p005/WS104完了 | [q531](../history/queue-q531.md) |
| [ws105-p007](../history/ws105/q532/phase.md) | compositor dma-buf/implicit sync | cleared | p006 | [q532](../history/queue-q532.md) |
| [ws105-p008](../history/ws105/q533/phase.md) | app build/data/IME | cleared | p007 | [q533](../history/queue-q533.md) |
| [ws105-p009](../history/ws105/q536/phase.md) | gdm/logind/D-Bus/VT | cleared | p008/p005修復 | [q536](../history/queue-q536.md) |
| [ws105-p010](../history/ws105/q537/phase.md) | WiFi/WPA/interface/ALSA | cleared | p008 | [q537](../history/queue-q537.md) |
| [ws105-p011](../history/ws105/q538/phase.md) | 全文規約/境界/両OS回帰/install文書/WS照合 | cleared | p001〜p010 | [q538](../history/queue-q538.md) |

依存は prerequisite → dependent: WS104/p001→p002→p003→p004→p005→p006→p007→p008→p009/p010→p011（p005/p006はp001/WS104完了も前提）。全ての必要出力を実装と証拠で確認。旧q526（p004）、q529（p006）、q534（p009）のunclearedは[全attempt索引](../history/ws105/index.md)と元Queueに保存、後続clearanceで書き換えていない。p005のq528/q530当時の結果も保持。

## 制限・移管・再利用

- [BUG-125](../bugs/BUG-125.md)/[BUG-127](../bugs/BUG-127.md): ユーザーの具体的tracking/clear許可。長いresize調査は取り止め、window Queueでbounded調査する。Linux移植との因果は未証明。
- [F-065](../future/F-065-keiland-portable.md): Linuxの合意範囲は完了。FreeBSDと[design§8](design.md#8-範囲の外ws105では作らないfuture-work-か後の-ws)は保留。実機/ARM/musl、他GPU/FOREIGN/tiled、旧kernel、explicit sync、EGL/GLES/browser、Qt/GTK、PCM feedbackなどを完了とはしない。
- cooperative logind pause通知/ACK、hardware hotplug/systemd restartは未実施。WiFi/ALSAは最初の対応deviceを使う。SIGKILLはWPA私有pathを残し得る。通常close/LogOut/SIGTERMはpaths0/error0/cleanup_failed0。
- [Linux install/起動文書](../../userland/desktop/LINUX.md)、[継続試験](../tools/keiland-linux/README.md)、[OS境界checker](../tools/keiland-os-boundary/check.sh)、[zedBSD手順](../tools/keiland-linux/zedbsd-commands.md)。dbus-wire.c/.py・seat-fd.cも継続道具へ移してMasterに登録。
- 証拠・scope snapshot・元Phase・surveyを検証してからPhase directory/WS専用tests/surveyを整理。WSにはws.md/design.mdを残す。最終sourceを後で変えた場合は該当full-standard scopeを再検証する。

## 同期・終了

WS105は**completed（local verified）**。GitHub Issue body/comment、Phase/WS intended close、Project projectionはoutboxで保留。remoteのclose/read-backは未実施。q538 finished、executor終了、own guest停止/overlay削除、pushなし。次のQueueは開始していない。完了根拠とpending publicationは別々に記録する。
