<!-- awesome-plan project=zedbsd record=ws131-p005 -->

# ws131-p005: backend の電源の領域

Status: in-progress（q650、2026-10-03、P1。実装と host の確認は済み、QEMU の試験は試験の担当に予約。元の記載: planning）
Disposition: normal
Parent: [WS131](../ws.md)、計画の正本 [design.md](../design.md)
Queue: q650（2026-10-03、Q1 の割り当て）
依存: p004 cleared。判断 D12
目安: 3〜4h（1 Queue）。実行者: Q1 が割り当てる（high）
所有 path: `userland/desktop/libkeiland-backend*/`（power、`unsupported/`）、`userland/desktop/wayland/greeter.c`（POWER の行の送り）、`plan/ws131/`

## 目的と結果

電源の状態と操作を backend の領域にする。zedBSD の greeter の電源断・再起動（今は `greeter.c:1088-1104` が sessiond に `POWER` の行を書く）を backend-zedbsd の power へ移す。Linux は logind の D-Bus（`PowerOff`・`Reboot`・`Suspend`、D-Bus が backend に移る p006 で有効にする）、FreeBSD と zedBSD の session の中は unsupported（sessiond は session の socket で POWER を受けない、`sessiond/session.c:309-324`、D12）。

## 範囲

1. interface に power（`kl_backend_power_get_state`・`kl_backend_power_action`、答えは `host->session_answer`、design.md §3.3・§3.4）。
2. zedBSD: greeter の `POWER` の行と答えの読みを backend-zedbsd へ（greeter の画面の code は compositor に残る）。電池の状態は今の source に無いので unsupported。
3. Linux: logind の電源の呼び出しは D-Bus の wire（今の `wayland/linux/dbus-linux.c`、p006 で backend へ移る）を使うので、この Phase の Linux の power は unsupported の枠にし、p006 で D-Bus と一緒に有効にする（backend が compositor の file を一時に共有する形を作らない）。FreeBSD: unsupported。
4. 答えは一度に一つ（§3.4）。

## 受け入れ

- 3 OS の build（zedBSD の amd64 は exit 0・自前の warning 0、`make keiland-linux` の gcc と clang、FreeBSD の native build と `native-build-audit.py`）と `plan/tools/keiland-os-boundary/check.sh` PASS。
- zedBSD: boot-test、`zdesktop-p101.sh` の image の greeter で電源の button（再起動）が sessiond に届く手順（使い捨ての guest）。Linux・FreeBSD: unsupported が返る host の試験と、compositor の起動の PNG（Linux）。

## 検証の方法

[zedbsd-commands.md](../../tools/keiland-linux/zedbsd-commands.md)、[keiland-linux/README.md](../../tools/keiland-linux/README.md)、[keiland-freebsd/README.md](../../tools/keiland-freebsd/README.md)。QEMU の console・serial の log で判定しない（不具合は gdbstub・monitor・QMP）。build は自分の `BUILD=build/ws131-pNNN/`、共有の `build/` を消さない。FreeBSD の起動の確認は判断 D11 の範囲（passthrough なし）。QEMU と実機の証拠を分ける。

## 衝突・危険・rollback

- 衝突: greeter.c は P2・WS099 の作業と重なりうる。 順は Q1 が決める（design.md §7.2）。
- rollback: 前へ直すのを基本にし、後ろに同じ file の変更が無い時だけ統合の commit を revert する（design.md §7.4）。

## Resume

依存の Phase の cleared と main への統合、関係する判断の決定の後に、Q1 が Queue を作る。

## 実施の記録（q650、P1、2026-10-03）

base は main（p004 の `b8b50d098` を含む）。

### 変えたこと

- `libkeiland-backend/keiland-backend.h`: power の領域。
  - `KL_BACKEND_POWER_POWEROFF`・`REBOOT`・`SUSPEND`、`KL_BACKEND_POWER_ACTION_BIT`、`KL_BACKEND_POWER_SOURCE_*`。
  - `struct kl_backend_power_state`（source・percent・charging・actions）、`kl_backend_power_get_state`・`kl_backend_power_action`。
  - `struct kl_backend_options` に `greeter_descriptor`（login screen の sessiond への descriptor、それ以外は -1）。
- `libkeiland-backend/backend-private.h`（新規）: `struct kl_backend` の中身を backend.c と OS の領域で共有する。`power_asked` で一度に一つ。compositor は include しない。
- `libkeiland-backend-zedbsd/power-zedbsd.c`（新規）:
  - login screen（`greeter_descriptor` ≥ 0）だけが poweroff・reboot を出す。session では actions 0・ENOTSUP（D12、sessiond は session の socket で POWER を受けない）。
  - `POWER poweroff|reboot` の 1 行を一度に書く。2 回目は EBUSY。
  - 電池の interface は無いので source は unknown。
- `libkeiland-backend/unsupported/power-unsupported.c`（新規、Linux・FreeBSD の共通）: actions 0、全ての action が ENOTSUP。Linux の logind は D-Bus と一緒に p006 で有効にする。
- compositor:
  - `main.c` は greeter の時に `--auth-fd` を `greeter_descriptor` に渡す。
  - `greeter.c` の `greeter_power_send` は行を自分で書かず `kl_backend_power_action` を呼ぶ。log の `ZWL GREETER power=…` は同じ。失敗は `ZWL GREETER send errno=N`。
  - sessiond の答え（OK）は、AUTH の答えと同じ descriptor で greeter.c が読むまま。読みの移動は session の領域（p006）で行い、`host->session_answer` もそこで足す。
- build: `sources.mk` に power-zedbsd.c、Linux・FreeBSD の backend の Makefile に power-unsupported.c。
- 試験（新規）: `plan/ws131/tests/host-power.c`・`host-power.sh`。zedBSD の実装は socketpair を sessiond の代わりにし、unsupported の実装と 2 回 build する（ASan・UBSan）。

### 確認（host）

| 確認 | 結果 |
| --- | --- |
| `host-power.sh` | zedBSD の実装 17/17、unsupported 6/6。中身: login screen の actions、`POWER reboot\n`・`POWER poweroff\n` の 1 行、2 回目の EBUSY、suspend と未知の action の ENOTSUP、sessiond が閉じた時の EPIPE、session の actions 0 と ENOTSUP、backend 無しの EINVAL |
| zedBSD の build（libkeiland.so・wayland・settings） | exit 0、warning 0。compositor は `kl_backend_power_*` 2 個、libkeiland.so は `kl_backend_*` を出さない |
| Linux の build（native の gcc・clang） | どちらも exit 0、warning 0。`libkeiland-backend.a` の未定義の symbol に `zwl_`・`kwl_`・`keiland_` が無い（B1）。header-check PASS（347）、install、elf-check PASS（24 ELF） |
| 境界の checker | `check.sh` PASS |
| FreeBSD | 書くだけ |

### QEMU の試験（試験の担当に予約、結果待ち）

- boot-test。
- criteria の image で `plan/ws099/tests/c1-boot-shutdown.sh`。greeter の Shut Down が backend を通って sessiond に届き、machine が止まる。`ZWL GREETER powering=poweroff` の後に `power=poweroff`。
