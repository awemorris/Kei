<!-- awesome-plan project=zedbsd record=ws131-p008 -->

# ws131-p008: backend の表示の領域

Status: uncleared（q650、2026-10-03、P1。実装と host の確認は済み、ユーザーのソフトな停止で QEMU・Linux の guest の試験は未依頼）
Disposition: normal
Parent: [WS131](../ws.md)、計画の正本 [design.md](../design.md)
Queue: q650（2026-10-03、Q1 の割り当て）
依存: p007 cleared
目安: 3〜4h（1 Queue）。実行者: Q1 が割り当てる（high）
所有 path: `userland/desktop/wayland/`（`zedbsd/os-zedbsd.c`、`linux/os-linux.c`・`freebsd/os-freebsd.c` の残り、`zwl-os.h`、`compose.c`・`display.c` の結線）、`userland/desktop/libkeiland-backend*/`、`plan/ws131/`

## 目的と結果

表示の node（Linux・FreeBSD の primary node）と Vulkan の display の取得・解放を backend へ移す。backend は compositor が渡す Vulkan の instance と proc addr だけを使う。

## 範囲

1. interface に display（`kl_backend_display_node`・`display_acquire`・`display_release`、`struct kl_backend_vulkan`）。
2. `git mv`: `zedbsd/os-zedbsd.c` と `os-linux.c`・`os-freebsd.c` の残り → 各 backend。`zwl-os.h` を除く。

## 受け入れ

- 3 OS の build（zedBSD の amd64 は exit 0・自前の warning 0、`make keiland-linux` の gcc と clang、FreeBSD の native build と `native-build-audit.py`）と `plan/tools/keiland-os-boundary/check.sh` PASS。
- zedBSD: boot-test、C1・C2・C9。Linux: KMS の確認（`display-probe.c`）と compositor の PNG。

## 検証の方法

[zedbsd-commands.md](../../tools/keiland-linux/zedbsd-commands.md)、[keiland-linux/README.md](../../tools/keiland-linux/README.md)、[keiland-freebsd/README.md](../../tools/keiland-freebsd/README.md)。QEMU の console・serial の log で判定しない（不具合は gdbstub・monitor・QMP）。build は自分の `BUILD=build/ws131-pNNN/`、共有の `build/` を消さない。FreeBSD の起動の確認は判断 D11 の範囲（passthrough なし）。QEMU と実機の証拠を分ける。

## 衝突・危険・rollback

- 衝突: WS113 p004（compositor の出力）と同時に流さない。 順は Q1 が決める（design.md §7.2）。
- rollback: 前へ直すのを基本にし、後ろに同じ file の変更が無い時だけ統合の commit を revert する（design.md §7.4）。

## Resume

依存の Phase の cleared と main への統合、関係する判断の決定の後に、Q1 が Queue を作る。

## 実施の記録（q650、P1、2026-10-03）— user のソフトな停止で区切り

2026-10-03 user「これによりすべての作業をソフトに停止します。」（Q1 経由）で、build が通る所で commit して止めた。

### 変えたこと

- 新規 `libkeiland-backend/keiland-backend-display.h`: `struct kl_backend_vulkan`（instance と `vkGetInstanceProcAddr`）、`kl_backend_display_node`・`acquire`・`release`、`kl_backend_display_socket`（その OS の既定の socket）。
- 新規 `libkeiland-backend/drm/display-drm.c`（Linux・FreeBSD）:
  - seat の primary の fd を `vkAcquireDrmDisplayEXT` に渡し、release は `vkReleaseDisplayEXT`。
  - 既定の socket は `XDG_RUNTIME_DIR`（無ければ /tmp）の `wayland-keiland`。もと `os-linux.c`・`os-freebsd.c` の中身で、二つの file は `git rm` した。中身はこの file に移っており、`git mv` ではない。
- `git mv wayland/zedbsd/os-zedbsd.c → libkeiland-backend-zedbsd/display-zedbsd.c`: node は ENOTSUP、acquire・release は何もしない、socket は compositor の既定のまま（ENOTSUP）。
- compositor:
  - 新規の共通の `wayland/os.c` が `zwl_os_*` を backend の上に実装する。seat の open（zedBSD の ENOTSUP は OK）、node の path を `KEILAND_DRM_DEVICE` に、既定の socket、poll の委譲、display の acquire・release。
  - `zwl-os.h` は削除した。宣言は `zwl.h`（open・close・poll）と `compose.h`（display）へ移した。
  - 1 つ差がある: FreeBSD の release の失敗は前は `server->failed` にしていたが、今は Linux と同じく log だけ（FreeBSD は書くだけ、まだ確かめていない）。
- build:
  - `sources.mk` を `KL_BACKEND_ZEDBSD_COMPAT_SOURCES`（network・audio と net の 3 つ）と全体に分けた。libkeiland.so は compat の分だけを compile する（前は backend の全部が入っていた。exports.map で出てはいなかった）。
  - compositor の 3 つの Makefile は os.c を足し、OS の os-*.c を外した。
- これで compositor の `wayland/linux/`・`freebsd/` に残るのは sync-*.c（p009）と apps.conf.in・keiland.desktop、`wayland/zedbsd/` は gpu の 3 つ（p009）だけ。

### 確認（host）

| 確認 | 結果 |
| --- | --- |
| zedBSD の build（libkeiland.so・wayland・settings） | exit 0、warning 0。libkeiland.so の `keiland_audio_*`・`keiland_network_*` は 19 個で変わらない |
| Linux の build（native の gcc・clang） | exit 0、warning 0。header-check PASS（360）、B1 は 0 |
| `check.sh` | PASS |
| host-session・power・seat-freebsd・seat-linux | PASS |
| QEMU（boot-test、C1）・Linux の guest（display-probe、compositor の PNG） | 未依頼（user の停止の指示で新しい試験は出さない） |
| FreeBSD | 書くだけ |

### 再開の条件

- user の再開の指示の後、試験の担当に次を依頼する。
  - zedBSD: boot-test と C1。
  - Linux: gdm の guest で compositor の PNG、`display-probe`（README の KMS の確認）。
- 結果を見て Q1 が判定する。
- その後 p009（GPU の buffer と境界の確定）へ進む。
