<!-- awesome-plan project=zedbsd record=guardrail -->

# Guardrail

zedBSD の貢献の規則と標準の索引。Queue・backlog・実行許可ではない。より新しく具体的なユーザーの決定が優先する。
規則の本文（エージェントの守ること）はリポジトリ直下の [AGENTS.md](../AGENTS.md) の「プロジェクトの規則」節にもある。

## 範囲と構造

- 作業は承認された Phase の範囲の中で行う。kernel（`src/`・`include/`）、bootloader、libc（`src/libc/`・`include/libc/`）、
  userland、platform、build/config、tools は所有が違う。file の追加や module の境界の変更の前に、現行のコードと Phase を確かめる。
- secondary queue は 2026-09-26 ユーザー指示で削除した（うまく実行できず、作業中のデータや成果は無い）。
- HAL: **API の変更（`include/hal/hal.h` の宣言・契約・HAL の責務）は具体的な差分ごとの事前承認**が要る。`src/hal/` の実装の変更
  （既存宣言の実装の修正・補完・最適化、arch 内部の header と struct）は承認なしで行ってよい
  （2026-09-25 ユーザー「HALの実装は勝手に修正してください。APIの変更のみ許可が必要です」。2026-09-12 の「HALの改変には許可が必要です」を
  この範囲に狭めた）。実装の変更は Phase の記録に差分の所在と検証を書く。hal.h に触れる差分はレビューできる形で plan に置き、承認まで適用しない。
  承認済みの差分は下の表。
- driver の配置と `drv_` の global symbol の方針を保つ。検証済みの refactor を古い試験の前提より信頼し、試験の側を直す。
- RTL8822B の `.inc` はライセンスを分けるために独立させており、**別の file のまま保つ**。
- kernel と libc（2026-09-23 ユーザー明確化）: kernel と libc はモノリシック。kernel・driver・HAL が include してよい libc の header は
  `libc/vulkan/*` だけ。ioctl・errno などの ABI は UAPI に分ける。kernel は標準 C の header 名を暗黙に読まず、libc の object を link
  しない（kcrt を使う）。`userland/desktop/libvulkan` は必須の構成要素。SPIR-V の compile は kernel 空間の driver が行う。この構成は変えない。
- **compositor は libvulkan だけを使う（2026-09-30 ユーザー）**: Keiland の compositor（zdesktop、`userland/desktop/wayland/`）は、GPU と表示を
  libvulkan（Vulkan の API と拡張）だけで扱い、GPU の UAPI（`include/uapi/gpu*.h`）を ioctl で直接呼ばない。入力の device の evdev の ioctl は
  対象の外。今残る直の ioctl（起動時の表示の問い合わせ、buffer の import の確かめ、fence の問い合わせ、表示の claim・release、Vulkan の無い
  予備の表示）は [WS103](ws103/ws.md) で移した（2026-10-01 完了。OS 固有の部分は macro でなく OS ごとの module `gpu-zedbsd.c` に閉じた（V1 の改訂、2026-09-30 夜 ユーザー承認）。確かめは `plan/tools/gpu-boundary/v1-check.sh`）。ユーザーの問い「私はKeilandコンポジターがlibvulkanのみを使用していると思っていたのですが、
  ioctlを使ってしまっているのですか？」への Q1 の説明の後、「規則にして今移す」を選んだ。
  同日の補い（ユーザー）:「どうしても最適化に必要なところは、opt-outできるようにマクロで囲めますか？必須機能では使っていない気がします。」→
  必須の機能は libvulkan だけで動かす。最適化のためにどうしても要る直の ioctl だけは、compositor の build の macro（例 `ZWL_GPU_DIRECT`、既定は有効）で
  囲み、macro を無効にした build でも全ての機能が動くようにする（WS103 の V1）。
  同日の決め（ユーザー）:「では、まずioctlを可能な限りやめて、Vulkan APIでlibvulkanで行うようにします。移行できない部分は、マクロで囲って、
  zedBSDでのみ行うようにします。evdevはLinuxにもあるので、ひとまずノータッチでよいです。」→ 直の GPU の ioctl はできる限り Vulkan の API（zedBSD では
  libvulkan）へ移す。移せない物は zedBSD の時だけ build される macro で囲む（Linux・FreeBSD の build では入らない）。evdev の ioctl は今は変えない。
- **Keiland の OS の境界（2026-10-01 ユーザー「Linux移植を進めます」、[WS104](ws104/ws.md)・[WS105](ws105/ws.md)。WS104 の完了から効く）**:
  desktop（libkeiland・compositor）の OS に固有の code は OS ごとの C の source に分け、`<package>/zedbsd/<役割>-zedbsd.c`・`<package>/linux/<役割>-linux.c`、
  Linux と FreeBSD で共有する仕組みは `<package>/<仕組み>/<役割>-<仕組み>.c` に置く。FreeBSD15 の新しい backend は `<package>/freebsd/<役割>-freebsd.c` に置く（WS109 の計画、未実装）。共通の source は `<uapi/...>`・`"userland/base/..."` を include せず、
  OS の macro の block は非常に細かい所だけ（今は `wayland/zwl-evdev.h` の 1 つ、ユーザー「非常に細かい部分ではマクロブロックで分けてよい」）。
  desktop の公開の header は `userland/desktop/keiland/`（`vulkan/`・EGL・GLES は OS の API として `include/libc/`）。install の path は `userland/desktop/paths.h` の macro。
  Linux の build は package ごとの `Makefile.linux`（zedBSD の build と完全に別、`make keiland-linux`）。確かめは `plan/tools/keiland-os-boundary/check.sh`（WS104 p008 で作る）。
- **browser / libbrowser（2026-10-01 ユーザーレビュー）**: engine の source/private header/table/shader/生成器は `userland/desktop/libbrowser/` が所有し、
  `userland/desktop/browser/` は libbrowser.so のコンポーネントを window/tab に包む main/shell/app data を所有する。
  libbrowser は標準 Vulkan を使用可、Wayland の header/API/protocol と直接の link は public/private とも使用不可。
  shell が Wayland events を抽象化した public input interface に変換する。全文の正本は [browser component](standards/browser-component.md)。
  新しい mandatory rule と配置の置換であり、C coding-style の例外ではない。[WS107](ws107/ws.md) で移行し、最終source/header/include/link/実clientを検証済み（2026-10-02）。
- **test app の配置（2026-10-01 ユーザーレビュー）**: base/desktop の対象30件（mview/gpudemo を含む）を `userland/tests/` へ移す計画は [WS106](ws106/ws.md)。
  対象表と package/config/install の契約を確定してから移動する。POSIX test utility は base に残す。
- kernel の実装を userland の build の依存へ写さない。`mkfs` などの tool は単独で使える形を保つ。
- base system の実装とライセンスの境界: [設計方針](master-design-policy.md)。
- 外部 package（`userland/packages/`）はソースツリーへ取り込まず、tarball を取得・検証して patch する。ライセンスは
  [provenance](ws032/provenance.md) と `plan/tools/packages/audit-licenses.sh` で監査する。
- WS は一つの具体的な到達目標を持つ（2026-09-12 ユーザー指示）。完了・終了した WS を再利用して別の目標を足さない。
  WS の終了時は子 Phase を全件照合し、未完了は完了にせず、指定の保留先か別の WS へ引き継いで元の Phase を閉じる。

## 標準と検証

- C のコーディング規約: [coding-style.md](coding-style.md)（全文が正本。簡約版は置いていない）。新しいコードには全文を適用する。
  規約の変更で評価順・所有・振る舞いを変えない。tool の対応: [standards/automation.md](standards/automation.md)。
- 集約の `make check` は走らせない。Phase に意味のある絞った確認を行う。
- build の関門: 選んだ platform の構成で `make -j16`（warning 0）。
- 起動の確認は `plan/tools/boot-test.sh`（画面の login prompt）。QEMU の console log・serial log を読んで判定しない。
  WS105 の Debian 13 Linux guest は SSH の疎通と QMP の `screendump` の PNG を用いる（2026-10-01 ユーザー許可、[WS105 D25](ws105/ws.md)）。SSH はホストの loopback から QEMU の転送を経て guest に接続する。zedBSD の image は `boot-test.sh` を使う。
  機能の回帰は guest を起動しない host の試験で行い、guest の操作はシリアル（`plan/tools/guest/serial.py`）か SSH
  （`plan/tools/guest/guest.sh`）で対話する。不具合の解析は QEMU の gdbstub・monitor・QMP で行う。詳細は [Master](master.md) の Tools 節。
- 回帰の範囲は Phase の性質で決める。コードの意味を変えない refactor は build（warning 0）と最後の boot test だけ。
- 表示の build（2026-09-29 ユーザー）: demo・実機の image は既定（logo を出し `kmsg=quiet`）。GPU の driver を直す Phase では logo を無効にし
  kernel の message を画面に残す（`ZEDBSD_GRAPHICAL_BOOT=n "ZEDBSD_BOOT_EXTRA_LINES=display=edp login=graphical"`）。
- ユーザーの受け入れの範囲を守る。取り下げられた網羅的な異常系試験・繰り返しの実機起動・免除された実機関門を戻さない。
  QEMU の証拠と実機の証拠を分けて書く。

## 承認済みの HAL 差分

| 日付 | 範囲 | 出典 |
| --- | --- | --- |
| 2026-09-12 | amd64 の MMIO read/write accessor 8 個（hal.h・責務は不変） | WS014 p003（q306） |
| 2026-09-13 | amd64 の device mapping の補完（patch SHA256 `e6ec9e6c2deda41b840fa6f10846438d091f3a20ce782b9251b7979ac7591c8d`） | WS030 p002（q308）、[承認記録](https://github.com/awemorris/zedBSD/issues/390#issuecomment-5647471812) |
| 2026-09-23 | HAL 配下の `#include` path の変更と、kernel/HAL 共通の compile flag（`-nostdlibinc`・`-fno-builtin`）。宣言・実装・責務は不変 | WS035 p004・p023・p034・p035 |
| 2026-09-23 | `HAL_TIMER_FREQUENCY` を arch ごとに（`include/hal/arch/<arch>.h`）、pc98 の PIT 入力 clock を BIOS `0:0501` bit 7 で選ぶ | WS040 p001・p005 |
| 2026-09-23 | i386 の `hal_mmio_read8`/`hal_mmio_write8`（既存宣言の補完） | WS036 |
| 2026-09-24 | rpi4 の framebuffer の console の font を PC/AT の 8x16 の複製へ（`src/hal/arm64/bsp-rpi4/` への font の追加と描画の置換。hal.h は不変） | WS044 p001 |
| 2026-09-24 | rpi4 の framebuffer に描いた後（文字の枠・カーソル・初回のクリア）に data cache をメモリへ書き出す（`src/hal/arm64/bsp-rpi4/framebuffer.c`、`hal_dcache_clean_range` を使う。hal.h は不変）。実機で画面に出ない不具合の修正。差分 `plan/ws044/proposed/rpi4-framebuffer-cache.diff`、承認「承認、すぐ進める」 | WS044 p006 |
| 2026-09-24 | rpi4 の framebuffer の大きさを firmware の設定（`TAG_GET_PHYSICAL`、config.txt の 1920x1080）から取り、cache の書き出しの後に `dsb sy`（`src/hal/arm64/bsp-rpi4/framebuffer.c`。hal.h は不変）。差分 `plan/ws044/proposed/rpi4-framebuffer-size.diff`、承認「承認、すぐ進める」 | WS044 p007 |
| 2026-09-24 | rpi4 の起動の診断: ACT LED（GPIO42）の点滅で段階を示す `led.c`/`led.h`、framebuffer の 3 秒のテスト模様と値の表示、`locore.S` で `SCTLR_EL1` を確定した値にする（MMU・cache 無効、little-endian）。hal.h は不変。差分 `plan/ws044/proposed/rpi4-boot-diagnostics.diff`、承認「承認、すぐ進める」 | WS044 p008 |
| 2026-09-24 | arm64 の命令 cache の同期: user の page を実行可能に対応付けるとき（`hal_space_map`）と実行可能に変えるとき（`hal_space_prot`）に `hal_sync_instruction_stream`、MMU の有効化で EL0 に UCT・DZE・UCI・nTWI・nTWE（`src/hal/arm64/space.c`・`locore.S`。hal.h は不変）。実機の init の SIGILL の修正。差分 `plan/ws044/proposed/arm64-icache-sync.diff`、承認「承認、すぐ進める」 | WS044 p009 |
| 2026-09-25 | amd64 の `amd64_percpu_current()` を `rdmsr IA32_GS_BASE` から `%gs:0` の `self` の load に（`src/hal/amd64/percpu.c`、`percpu.h` に `self` が先頭の field である `_Static_assert`。hal.h は不変）。lock と `thread_current()` のたびの rdmsr が kernel の時間の約 24% だった。差分 `plan/ws046/phase009/hal-percpu-gs.md`、承認「承認待ちのHALの変更を許可します」 | WS046 p009 |

| 2026-09-25 | amd64 の page table の owner PTE に子 table の present entry の数を bit 52〜62 に持ち、`hal_space_unmap` の空 table の切り離しを全 table の走査（`detach_empty_tables`）から O(1) の判定に（`src/hal/amd64/space.c`。hal.h は不変）。差分 `plan/ws061/phase002/hal-amd64-table-counts.diff`。2026-09-25 の規則の変更（実装は承認不要）により適用 | WS061 p002 |
| 2026-09-27 | arm64 の `hal_pmem_map_uncached()`・`hal_pmem_unmap_uncached()`（hal.h に宣言を追加、arm64 だけ実装: MAIR の entry 3 を Normal non-cacheable、`0xffff_0080_0000_0000` の uncached の窓、`src/kern/uncached.c`）。PCIe の DMA が cache を snoop しない Pi 4 の xHCI のため（`plan/ws048/proposed/hal-pmem-uncached.diff`） | WS048 p004。ユーザー「HAL approvalsは3つとも承認します。」 |
| 2026-09-27 | amd64 の `hal_get_arch_handoff("acpi.rsdp")`: HAL の ACPI の発見が受け入れた RSDP の物理 address を返す（hal.h は不変、HAL の責務の追加。`plan/ws049/proposed/hal-acpi-rsdp.diff`） | WS049 p006。同上 |
| 2026-09-27 | aarch64 の `include/hal/arch/aarch64.h` に `hal_gpregs`・`hal_fpregs`・`hal_vregs` と `HAL_DEBUG_*`（amd64 と同じ形）、`src/hal/arm64/debug.c`（single step、hardware breakpoint・watchpoint、context switch ごとの debug state）。ptrace のため（commit 27831f19） | WS044 p010。同上 |
| 2026-09-28 | amd64 pcat の `src/hal/amd64/bsp-pcat/cons.c`: `kmsg=quiet` のとき framebuffer を消さず、右上の 136x40 の進捗の枠を logo の背景で塗り、`console_suspended=1` にする（HAL の責務の変更。hal.h は不変） | ユーザー「HALのdiffは承認します。」（2026-09-28） | `plan/ws035/proposed/hal-quiet-console.diff`（SHA256 3f63a8411d5b3684c8bb790e86e049a0d70122626c6f1aca579e5413cbb8566b） |

2026-09-25 以降、hal.h を変えない `src/hal/` の実装の変更は承認を要しない（上の規則）。hal.h の変更はこの表の承認が要る。

## 決定の出典

ユーザーの指示（HAL の制限と rollback の review、RTL8822B のライセンスの例外、refactor と試験の信頼、WS025 の規約の柔軟性、
インストーラの受け入れの範囲、QEMU だけの UAS の受け入れ）。新しい規則はこの索引、該当する規約の全文、tool の対応、影響する計画を
更新する。合意した構造や範囲を黙って置き換えない。

- WS106 の純粋なsource移動には [既存style維持の限定例外](standards/ws106-relocation.md)を適用（2026-10-01 ユーザー回答）。新実装/意味変更は対象外、全文reviewとhash/diff/build/install/bootは必要。WS106完了時に適用を終了。

WS107 の限定例外（2026-10-02ユーザー承認）: [全文](standards/ws107-relocation.md)。内容不変の移動styleのみ保持、component品質修正/新試験/既知14候補はC全文適用、完了時失効。

WS108（2026-10-02 user）：Debian13/Ubuntu26.04のQEMU guest作成・native build/dpkg導入/GUI検証、loopback SSH/QMP PNGで起動確認を許可。WS105の既存SSH/PNG方式をこの2OSへ適用、serial/console log判定禁止、host画面/入力/optを変更しない。CIのmake targets/release filesを作成、pushと実remote publishは行わない。

WS108 release/native検証の再利用tool: [driver/inputs/手順](../tools/release/keiland-linux-deb/README.md)、[検証coverage](standards/automation.md#ws108-packaging-coverage-2026-10-02)。2026-10-02指示を実装・検証、C標準/既存OS/ABI方針は不変。

## WS109 native FreeBSD の承認（2026-10-02）

ユーザー「drm-kmodを利用OKです。FreeBSDにも例外を適用します。FreeBSD実機は用意しておくので、作業を進めておいてください。」（2026-10-02 JST、このchat）。D1:既存FreeBSD drm-kmod利用可、GPL-free systemstack条件をこの範囲で置換。Keiland sourceの寛容license/外部実装を取り込まない境界は維持。D2:WS109専用FreeBSD QEMU guestのloopback SSH/QMP PNG検証を承認。D3:実機はユーザーが準備、入手前にnative build/backend実装を進める。実GPU/WiFi結果は将来の実機関門に残し、mock/QEMUbuildで代替しない。

適用の全文: [WS109 native scope](standards/ws109-native.md)。C規約の例外ではない。実機のdevice名/driver/検証結果は到着後に記録し、WS acceptance F1/F3/F4/F5の残る部分を検証する。

WS109 q557 native input classification: Linux/FreeBSD share the evdev discovery/read and native
EVIOCGABS/EVIOCGNAME/EVIOCGID/EVIOCGBIT/EVIOCSCLOCKID mechanism in wayland/evdev, with native
record/ioctl constants chosen only by the approved tiny zwl-evdev.h selector. Seat/VT/device
authority stays in OS modules; dma-buf reservation export ioctl also stays in OS modules.
This realizes the existing shared-mechanism rule for the authorized FreeBSD port; public
contracts/device operations and zedBSD GPU/ioctl constraints are unchanged. Boundary checker
recognizes freebsd roots and only those existing evdev request families in that one source.

## 2026-10-02 / ws109-20261002-qemu-venus-acceptance

Current user, this chat async reply: 「実機検証は不要です。qemuでVenusが使えればclearとします。」
This explicitly replaces the earlier user-provided-machine gate. Physical GPU/display/WiFi
acceptance is waived for WS109; do not request hardware or reintroduce those gates. Required
replacement evidence is actual FreeBSD QEMU Venus usage; mere host support, headless lavapipe or
Linux/zedBSD Venus does not establish that evidence. Native backend/build/standards and affected
regression obligations remain. p004 hardware radio operations become waived, not falsely tested;
native audio/wired and native radio ABI/refusal/WPA wire evidence remains classified accurately.
p003/F3 and p005/F5 replace physical display/main-app checks with owned FreeBSD QEMU Venus-backed
checks. If the native guest stack lacks a required Venus driver, investigate a bounded actual
capability chain and expose the remaining platform/scope choice; kernel/driver port is still outside
WS109's agreed scope. Current q566 standards/regression/docs subset stays authorized; Venus
configuration/implementation is selected separately after q566. No automatic WS/Phase clearance.
Origin user decision reconciled to WS/all changed Phase own criteria, Guardrail/scoped standard,
Queue supplement and docs; remote decision/structural events pending publication.

## 2026-10-02 / ws109-20261002-user-i915-passthrough

Current user chat reply: 「awe@10.0.10.25 でi915をPCIパススルーして、FreeBSDゲストを実行してみましょう。」
This authorizes SSH to that specified host and an owned FreeBSD QEMU guest with existing
IrisXe0000:00:02.0 passthrough. The prior loopback-only rule has a scoped host-control exception;
guest SSH remains via remote127.0.0.1 forward, QMP PNG and real native observations, no serial
logs. Physical WiFi/user-supplied-machine request remains waived. Try this explicit native i915
GPU route to address q567 Venus prerequisite; boot alone does not clear F3/F5 or claim Venus.
Remote survey: chaos/Linux6.19.13/QEMU10.0.11/7.4GiBmemory; GPU8086:46a8 alreadyvfio-pci,
IOMMUgroup0 GPUalone, no other QEMU. Use4GiB guest, owned disk copy/overlay/endpoint. No host
GPU unbinding/reboot/kernel/library replacement or unrelated VM/process changes. Existing
native i915/drm-kmod may be installed in own guest under prior permission. New driver port
still excluded. Actual native Vulkan/DRM/provider/render/lease checks required where possible.
