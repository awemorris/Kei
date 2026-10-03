<!-- awesome-plan project=zedbsd record=ws131-p005 -->

# ws131-p005: backend の電源の領域

Status: planning（p002 第 2 版はユーザーのレビュー済み（2026-10-03、D7 は確認中）。開始はユーザーの承認と P2 の終了の後に Q1 が指示）
Disposition: normal
Parent: [WS131](../ws.md)、計画の正本 [design.md](../design.md)
Queue: none
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
