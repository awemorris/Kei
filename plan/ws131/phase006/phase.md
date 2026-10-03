<!-- awesome-plan project=zedbsd record=ws131-p006 -->

# ws131-p006: backend の seat・session の領域

Status: planning（p002 第 2 版はユーザーのレビュー済み（2026-10-03、D7 は確認中）。開始はユーザーの承認と P2 の終了の後に Q1 が指示）
Disposition: normal
Parent: [WS131](../ws.md)、計画の正本 [design.md](../design.md)
Queue: none
依存: p005 cleared。判断 D10・D11
目安: 4〜5h（1 Queue）。実行者: Q1 が割り当てる（high）
所有 path: `userland/desktop/wayland/`（`linux/seat-*.c`・`dbus-linux.*`・`os-linux.c` の VT と seat、`freebsd/seat-freebsd.*`・`os-freebsd.c` の seat、`zedbsd/handoff-zedbsd.c`、`session/`、`greeter.c` の AUTH・UNLOCK、`main.c` の結線）、`userland/desktop/libkeiland-backend*/`、`plan/ws131/`。（Q1 の委任が要る: Q1 が Phase の前に委任を記録する）: `plan/tools/keiland-linux/dbus-wire.c`・`seat-fd.c` の path

## 目的と結果

seat と session（logind・直の seat・seatd・VT・sessiond の READY/GO/RELEASED/LOGOUT/QUIT・AUTH・UNLOCK）を backend へ移し、compositor の内部（`zwl_compose_*`・`zwl_input_close`・`server->os_paused`…）を backend が触らない形にする（design.md §3.4 の約束）。

## 範囲

1. 最初の 30 分で `handoff-zedbsd.c`・`greeter.c`・`seat-logind-linux.c`・`seat-freebsd.c` の状態の遷移を読み、callback の順序（pause の中で出力を止める順、`PauseDeviceComplete` を送る時、`ResumeDevice` の新しい fd、RELEASED の前の表示の解放、lock 中の UNLOCK の答え）を phase.md に表で書いてから移す。
2. interface に session（`ready`・`released`・`logout`・`authenticate`・`unlock`）と host の `session_paused`・`session_resumed`・`session_stop`・`session_answer`・`input_revoked`。答えの多重化は一度に一つ、`session_answer(request, error)`（§3.4）。
3. `git mv`: zedBSD の `handoff-zedbsd.c` と greeter の AUTH・UNLOCK の行、Linux の `seat-logind-linux.c`・`seat-direct-linux.c`・`seat-linux.h`・`dbus-linux.*`・`os-linux.c` の VT と seat、FreeBSD の `seat-freebsd.*`・`os-freebsd.c` の seat、`session/handoff-session.c` → `libkeiland-backend/session/`。Linux の power（p005 の枠）を logind の `PowerOff`・`Reboot`・`Suspend` で有効にする。
4. compositor: pause・resume・revoke の時の出力の停止と入力を閉じる処理を callback の中へ。

## 受け入れ

- 3 OS の build（zedBSD の amd64 は exit 0・自前の warning 0、`make keiland-linux` の gcc と clang、FreeBSD の native build と `native-build-audit.py`）と `plan/tools/keiland-os-boundary/check.sh` PASS。
- compositor の tree に seat・session・handoff の OS の code が無い（B1）。
- zedBSD: boot-test、C1・C2・C9、`zdesktop-p101.sh`（log out と login の受け渡し）、`zdesktop-p102.sh`〜`p104.sh`（lock と unlock）。Linux: logind の seat と直の seat の両方で起動、VT の切り替えの後の復帰、`seat-fd.c`・`dbus-wire` の fixture、使い捨ての guest で logind の Reboot の要求の後に guest が再起動する（SSH の切断と再接続）。

## 検証の方法

[zedbsd-commands.md](../../tools/keiland-linux/zedbsd-commands.md)、[keiland-linux/README.md](../../tools/keiland-linux/README.md)、[keiland-freebsd/README.md](../../tools/keiland-freebsd/README.md)。QEMU の console・serial の log で判定しない（不具合は gdbstub・monitor・QMP）。build は自分の `BUILD=build/ws131-pNNN/`、共有の `build/` を消さない。FreeBSD の起動の確認は判断 D11 の範囲（passthrough なし）。QEMU と実機の証拠を分ける。

## 衝突・危険・rollback

- 衝突: P2・WS113 と `main.c` を取り合う。 順は Q1 が決める（design.md §7.2）。
- rollback: 前へ直すのを基本にし、後ろに同じ file の変更が無い時だけ統合の commit を revert する（design.md §7.4）。

## Resume

依存の Phase の cleared と main への統合、関係する判断の決定の後に、Q1 が Queue を作る。
