<!-- awesome-plan project=zedbsd record=ws131-p007 -->

# ws131-p007: backend の入力の領域

Status: planning（p002 第 2 版はユーザーのレビュー済み（2026-10-03、D7 は確認中）。開始はユーザーの承認と P2 の終了の後に Q1 が指示）
Disposition: normal
Parent: [WS131](../ws.md)、計画の正本 [design.md](../design.md)
Queue: none
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
