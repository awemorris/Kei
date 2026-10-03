<!-- awesome-plan project=zedbsd record=ws131-p007 -->

# ws131-p007: backend の入力の領域

Status: uncleared（q650、2026-10-03、P1。T1-044 で 3/4 PASS、touch・pen の demo-s8-s9 はラップアップで未実行）
Disposition: normal
Parent: [WS131](../ws.md)、計画の正本 [design.md](../design.md)
Queue: q650（2026-10-03、Q1 の割り当て）
依存: p006 cleared。判断 D3
目安: 3〜4h（1 Queue）。実行者: Q1 が割り当てる（high）
所有 path: `userland/desktop/wayland/`（`zedbsd/input-zedbsd.c`、`evdev/`、`linux/input-seat-linux.c`、`zwl-input.h`・`zwl-evdev.h`、`seat.c`・`input.c`・`main.c` の結線）、`userland/desktop/libkeiland-backend*/`、`plan/ws131/`

## 目的と結果

evdev の device の走査・読み・lease を backend へ移す。evdev の型の選択（今の `zwl-evdev.h`、唯一の macro の block）を `libkeiland-backend/keiland-backend-evdev.h` へ（D3）。

## 範囲

1. interface に input（scan → `host->input_found`、absinfo・name・id・read・close）。
2. `git mv`: `zedbsd/input-zedbsd.c` → backend-zedbsd、`evdev/*` → `libkeiland-backend/evdev/`、`linux/input-seat-linux.c` → backend-linux。走査の時刻（`main.c:654-655`）と開いている fd の表は compositor に残る。
3. checker: C1・C2・L1 の evdev の例外の場所を backend の header と `libkeiland-backend/evdev/input-evdev.c` へ。

## 受け入れ

- 3 OS の build（zedBSD の amd64 は exit 0・自前の warning 0、`make keiland-linux` の gcc と clang、FreeBSD の native build と `native-build-audit.py`）と `plan/tools/keiland-os-boundary/check.sh` PASS。
- compositor に `zwl-input.h`・`zwl-evdev.h`・evdev の ioctl が無い。
- zedBSD: C1・C2・C9（pointer・key・touch・pen）、`demo-s8-s9.sh`、boot-test。Linux: QMP の pointer・key の入力で compositor が反応する PNG。BUG-111（repeat）・touch の時刻の補正に触れない。

## 検証の方法

[zedbsd-commands.md](../../tools/keiland-linux/zedbsd-commands.md)、[keiland-linux/README.md](../../tools/keiland-linux/README.md)、[keiland-freebsd/README.md](../../tools/keiland-freebsd/README.md)。QEMU の console・serial の log で判定しない（不具合は gdbstub・monitor・QMP）。build は自分の `BUILD=build/ws131-pNNN/`、共有の `build/` を消さない。FreeBSD の起動の確認は判断 D11 の範囲（passthrough なし）。QEMU と実機の証拠を分ける。

## 衝突・危険・rollback

- 衝突: P2 と `seat.c`・`input.c` を取り合う。 順は Q1 が決める（design.md §7.2）。
- rollback: 前へ直すのを基本にし、後ろに同じ file の変更が無い時だけ統合の commit を revert する（design.md §7.4）。

## Resume

依存の Phase の cleared と main への統合、関係する判断の決定の後に、Q1 が Queue を作る。

## 実施の記録（q650、P1、2026-10-03）

途中で BUG-161（ws100-p012）を先にするよう Q1 に言われ、未完の差分を patch に退避して後で当て直した（`build/p1-q640/p007-wip.patch`、commit はしていない）。

### 変えたこと

- `git mv wayland/zwl-evdev.h → libkeiland-backend/keiland-backend-evdev.h`。OS の macro の block（D3）はここだけになった。
  - 同じ header に `struct kl_backend_input_caps`（もと `zwl_input_caps`）と `KL_BACKEND_INPUT_PATH_MAX` を置いた。
  - 入力の関数: `kl_backend_input_scan`・`absinfo`・`name`・`id`・`read`・`close`。
  - compositor は evdev の型と code のためにこの header だけを include する。
- host の callback に `input_known(path)`（すでに読んでいる device を飛ばす）と `input_found(descriptor, path, caps)` を足した。input_found は 1 なら compositor が持つ、0 なら backend が閉じる。
  - 走査の中から呼ぶ（header に書いた）。callback から backend を呼ばない約束は保つ: 断られた device は backend が閉じる。
- 移動:
  - `git mv wayland/zedbsd/input-zedbsd.c → libkeiland-backend-zedbsd/`: 直に open、close。
  - `git mv wayland/evdev/input-evdev.c → libkeiland-backend/evdev/`（Linux・FreeBSD）: seat を通して open し、`EVIOCSCLOCKID`、seat の paused の間は走査しない。
  - 中身は機械的に変えた: compositor の server と inputs[] を見る所を callback に替えた。
  - `wayland/zwl-input.h`・`evdev/seat.h`・`evdev/seat-backend.c` を削除した。
- compositor:
  - `input.c` に `zwl_input_scan`（走査の時刻と backend の走査）を置いた。
  - `zwl_input_probe` は持つかどうかを返す。`attach_*` と `zwl_input_attach` は失敗の時に descriptor を閉じず、呼んだ側（backend）に返す。
  - 読みの glue（`input_read`）: ENODEV の時に `kl_backend_seat_device_revoked` に聞き、logind が残すなら record の fd を -1 にして EAGAIN（ws105-p009 の補正、p006b では evdev の adapter にあった）。
  - `tablet.c`・`touch.c` は `kl_backend_input_absinfo`・`name`・`id`。
  - `backend-host.c` に `zwl_backend_input_known`・`found`、`main.c` で結線した。
  - 走査の 2 秒ごとの時刻と、開いている fd の表は compositor に残る。
- build:
  - zedBSD の compositor の一覧から input-zedbsd.c を外し、sources.mk へ。
  - Linux・FreeBSD の compositor の一覧から evdev の 2 つを外し、backend の Makefile へ input-evdev.c を足した。
- checker（委任）:
  - C1 の zwl-evdev.h の例外と、C2 の input-evdev.c の ioctl の例外を外した（compositor と libkeiland に OS の include と ioctl は 0 個）。
  - L1 の例外を `libkeiland-backend/keiland-backend-evdev.h` に移した。

### 確認（host）

| 確認 | 結果 |
| --- | --- |
| zedBSD の build（libkeiland.so・wayland・xserver） | exit 0、warning 0 |
| Linux の build（native の gcc・clang） | exit 0、warning 0。header-check PASS（357）、B1 は 0、compositor は `kl_backend_input_*` 6 個 |
| `check.sh` | PASS（C1・C2 は例外なしで 0） |
| makefile-sync | 既存の誤検出（apps.c）だけ |
| `host-session`・`host-power`・`host-seat-freebsd`・`host-seat-linux` | 31/31、17/17・6/6、13/13、12/12 |
| 入力の host 試験 | 新しく作らなかった。走査・分類は実 device が要るので、QEMU で確かめる |
| FreeBSD | 書くだけ |

### QEMU と Linux の guest（試験の担当に予約、結果待ち）

- zedBSD:
  - boot-test。
  - C1。C2・C9 は user の指示で実機の試験の後。
  - pointer・key・touch・pen の `demo-s8-s9.sh`。
- Linux: gdm の guest で QMP の pointer・key の入力に compositor が反応する PNG（T1-042 の試験 2 の手順の click）。

## 結果（Q1、2026-10-03、T1-044、QEMU）

uncleared（未実行の項目が残る）。boot-test PASS、C1 PASS（p126・c1-boot-shutdown）、Linux の gdm の guest（QEMU+KVM）PASS（`ZWL INPUT device=` 4 つ、launcher の click で App Home、Terminal に `$ ab`）。demo-s8-s9（touch・pen）はユーザーの指示のラップアップで未実行。証拠 worktrees/t1/build/t1-criteria/t1-044/・t1-linux/t1-044/。再開: demo-s8-s9 を流して PASS なら cleared。
