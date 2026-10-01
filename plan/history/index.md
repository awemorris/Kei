<!-- awesome-plan project=zedbsd record=past-log -->

<!-- awesome-plan-current:start -->
Active Queue: なし
Last finished Queue: [q525](queue-q525.md)（ws105-p003 cleared）
<!-- awesome-plan-current:end -->

# Past Log

## 最新: 2026-10-01 q525 / ws105-p003

Linux 専用 libvulkan-compat を実装。後段への F 222 関数、I 12 関数、禁止する WSI N 51 関数を maintained TSV から生成。dispatchable handle を包まず、後段の dlsym の trampoline を使い、instance / physical-device / device / queue の ownership を保持する。mutex・pthread_once・pthread の thread-local record で publication / 再入 / lifetime を扱う。

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


ユーザー「ws105の完了をゴールにして、自走をお願いします。」により既存範囲を実行。実装 `5dcb10992baf2ae7b13e0d9f1af61b9b3a366d1f`、GitHub 未公開・push なし。[q525](queue-q525.md)。

## 最新: 2026-10-01 q524 / ws105-p002

Linux の独立 build の土台と、8 package の `Makefile.linux`（7 shared library と install しない digest archive）を実装。仮 network / audio backend は公開 API の署名を保持し、service 不在を返す。DNS は実際の `/etc/resolv.conf` の dotted IPv4 を読む。

- gcc / clang の最終 build exit 0、`-Werror`・warning 0。host gcc 14.2.0 / clang 19.1.7、GNU Make 4.4.1。target toolchain 変更無し、host package 追加無し。
- install した 7 ELF の `RUNPATH [/opt/keiland/lib]` と SONAME / NEEDED を確認、`elf-check: PASS (7 ELF)`。digest archive は stage に無い。
- source token の比較 `makefile-sync: PASS`、system-inclusive dependency の確認 `header-check: PASS (57 sources)`。system の Wayland / EGL / GLES header 混入無し。
- `lib-smoke: PASS`（version 21、network record / unreachable state、audio unavailable、resolver reader）。
- `make keiland-linux-clean` exit 0。Linux の lib / obj / stage を消し、p001 の guest.img は保持。
- `timeout 600 make -j64 disk-image`: exit 0、warning 0。Linux Makefile は target build に include されず、zedBSD source / toolchain に変更無し。共通 C source を直していないため runtime 回帰対象無し。
- 新規 4 C file は clang-format19（ColumnLimit 0）後、定義引数・3 条件の行を全文規約に従い復元。style-check total 0、手動全文レビュー、sh syntax、`git diff --check` PASS。仮 backend の常に拒む API の最終 return は規定の errno を保持。

証拠: `build/ws105-p002/` の gcc/clang（初回・最終）log、install/clean/zedbsd log、verify.log。永続の試験 summary と source manifest は `plan/history/ws105/q524/`。libvulkan と compositor / app は後続 Phase。host の `/opt/keiland` に install していない。host 試験は `KEILAND_DRM_DEVICE=none`、Wayland/X 環境を外し、timeout 付きで実施。


ユーザー「ws105の完了をゴールにして、自走をお願いします。」により既存範囲を実行。実装 `c1c9e48ea7ed636d62f3fb20e0c4179423d5a614`、GitHub 未公開・push なし。[q524](queue-q524.md)。

## 最新: 2026-10-01 q523 / ws105-p001

Debian 13 の base guest と操作の道具を作成。`guest.sh` は小さな sh 入口から Python の controller を呼ぶ（QMP JSON と座標変換を shell escaping 無しで扱う）。依存 package は既存 host にあり、host package 追加なし。

- `timeout 600 sh .../build-guest.sh`: exit 0。mmdebstrap 121.9975 秒、raw ext4 8 GiB。mesa-vulkan-drivers 25.0.7-2+deb13u1 / libvulkan1 1.4.309.0-1 / weston 14.0.2-1 / linux-image-amd64 6.12.107-1。guest kernel 6.12.107+deb13-amd64。
- `timeout 200 .../guest.sh start`: 180 秒以内に `guest: ready`。root と kei の loopback SSH 成功。kei の audio/video/input/kvm/render/netdev、`/run/user/1000` を確認。
- DRM card0 / ALSA controlC0 / event0〜5、Vulkan llvmpipe、mac80211_hwsim wlan0/wlan1、ALSA `'Master'` を確認。
- QMP PNG の login prompt を目視・ユーザーに提示。png-probe の size `1280 800`、2 点の色 `#000000`。key / click / type が QMP error 無し。
- run2 / port2226 の同時起動・SSH・停止 PASS。両 guest 停止後 overlay 無し、base image の size / mtime は一致。build の再実行は既存 image を保持。
- 追加の file 転送 / install 確認: 専用 stage の probe.txt を guest に install し、get 後 cmp 一致。guest 停止済み。host の `/opt` は変更していない。
- sh syntax、Python compile、`git diff --check` PASS。Master Tools 登録済み。共通 product code の変更無し、zedBSD 回帰対象無し。

証拠: `build/ws105-p001/`（build.log、start.log、verify.log、devices.txt、user.txt、image-before/after.txt、received.txt）。永続 PNG・版・試験 summary は `plan/history/ws105/q523/`。gdm variant の実行は p009、compositor / app の動作は後続 Phase。console / serial log は読んでいない。


ユーザー「ws105の完了をゴールにして、自走をお願いします。」により既存範囲を実行。実装 `39a0941c272ae3da5091efcba14907610ba5683b`、GitHub 未公開・push なし。[q523](queue-q523.md)。

## 最新: 2026-10-01 q522 / WS104 完了

q522 は WS104 の全文規約の最終 source の確認、OS の境界 checker、全体回帰、完了の整理を実行。ws104-p008 cleared、WS104 は A1〜A6 を確認して completed。
最終 amd64 build exit 0・自前 warning 0、全文規約の変更範囲違反 0（理由つきの保持と tool 誤検出は standards-review.md）。境界 C1〜C5 PASS、故意の uapi include は C1 FAIL / exit 1、同一に復元。GPU V1 54 source、dedicated host 18 case × ordinary / sanitize、decode host 17 case × ordinary / sanitize、forge / fence guest（600 fence、generation 1、600 frame / 64 s）PASS。compositor C1/C2/C9 は 13/13 PASS、glass p059・Notes pen/PDF・Settings host / guest 8 本・host audio 14/14・音量 p004/p005 PASS。必須 PNG は目視し、boot の login PNG をユーザーに提示した。実機・Linux compositor・他 platform は未実施。証拠と各試験の summary は plan/history/ws104/q522/。
実装 `cd48e74d110a1e504c2b87ac288d41cdc928367c`（WIP）。詳細の検証・例外・制限・exact scope は [q522](queue-q522.md) と [WS104](../ws104/ws.md)。

ユーザーの WS104 完了までの自律実行指示により q516〜q522 を依存順に実行した。q515 は別の exact Queue 承認。
Focus は fg012（WS104 → WS105）、fg010 の実機デモも保持。後続 WS105 は planned、実行は未開始。
実機 5330・他 platform・Linux compositor は未実施。commit は全て WIP、push / GitHub 公開は未実施、outbox に記録と event を保持。

## Queue history（直近 30、古い順）

| Queue | 目的（scope・attempt・結果はリンク先） |
| --- | --- |
| [q493](queue-q493.md) | Queue q493: framebuffer object と renderbuffer（ws068-p022） |
| [q494](queue-q494.md) | Queue q494: cube map（ws068-p023） |
| [q495](queue-q495.md) | Queue q495: OpenGL ES 3.0 の API（1）（ws068-p024） |
| [q496](queue-q496.md) | Queue q496: Remacs 用 host Noct の make 失敗 |
| [q497](queue-q497.md) | Queue q497: Remacs の Noct patch の再適用を防ぐ |
| [q498](queue-q498.md) | Queue q498: smoke を外し通常の make を完走する |
| [q499](queue-q499.md) | Queue q499: Windows Venusの実画面 |
| [q500](queue-q500.md) | Queue q500: WS074 Chromium比較手順 |
| [q501](queue-q501.md) | Queue q501: Amazonトップのpercentage height |
| [q502](queue-q502.md) | Queue q502: 動的に挿入された外部script |
| [q503](queue-q503.md) | Queue q503: Amazonの後続scriptが使うWeb API |
| [q504](queue-q504.md) | Queue q504: Amazon検索欄の文字の位置 |
| [q505](queue-q505.md) | Queue q505: 公開サイトの固定比較corpusと一般化修正 |
| [q506](queue-q506.md) | Queue q506: 複数の公開siteの画像・DOM比較とlayout改善 |
| [q507](queue-q507.md) | Queue q507: WS074 Acid2 exact rendering |
| [q508](queue-q508.md) | WS103（compositor を libvulkan だけにする）の調査と設計。 |
| [q509](queue-q509.md) | WS103 の p002（compositor の起動の問い合わせを VK_KHR_display へ、`--direct` の削除）。 |
| [q510](queue-q510.md) | WS103 の p003（libvulkan: VK_KHR_dedicated_allocation と VK_KHR_get_memory_requirements2、image の cap… |
| [q511](queue-q511.md) | WS103 の p004（compositor を dedicated の import に切り替え、`GPU_RESOURCE_IMPORT`・`GPU_RESOURCE_DESTROY` を… |
| [q512](queue-q512.md) | WS103 の p005（libvulkan の WSI が、Wayland の target の present ごとに新しい fence を作って送る）。 |
| [q513](queue-q513.md) | WS103 の p006（compositor の fence を poll だけに、`/dev/gpu0` と `--gpu` の削除、GPU の UAPI を `gpu-zedbsd.c` … |
| [q514](queue-q514.md) | WS103 の p007（規約の全文で WS の全 source の変更を見直す、回帰、5330、V4 の性能の計測）。WS103 の最後の Phase。 |
| [q515](queue-q515.md) | desktop の公開ヘッダーを libc から分離し、WS104 と WS105 の開始条件を整える。 |
| [q516](queue-q516.md) | audio の漏れを libkeiland へ（`keiland_audio_available`） |
| [q517](queue-q517.md) | libkeiland の OS の 3 file を `libkeiland/zedbsd/` へ |
| [q518](queue-q518.md) | compositor の GPU の buffer の境界を引き上げる |
| [q519](queue-q519.md) | compositor の入力の device の層を `wayland/zedbsd/input-zedbsd.c` へ |
| [q520](queue-q520.md) | compositor の session と OS の hook を zedBSD の module に |
| [q521](queue-q521.md) | install の path を `userland/desktop/paths.h` の macro に |
| [q522](queue-q522.md) | 規約の全文の見直し、境界の確かめの script、回帰 |
| [q523](queue-q523.md) | ws105-p001 cleared |
| [q524](queue-q524.md) | ws105-p002 cleared |
| [q525](queue-q525.md) | ws105-p003 cleared |

以前の全要約・古い Queue の index・判断・bug への参照は [q522 までの Past Log](past-log-through-q522.md) に保持。WS104 の Phase は history/ws104/q515〜q522 へ保存済み。
