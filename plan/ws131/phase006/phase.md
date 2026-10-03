<!-- awesome-plan project=zedbsd record=ws131-p006 -->

# ws131-p006: backend の seat・session の領域

Status: in-progress（q650、2026-10-03、P1。範囲 1 の読みと表は済み。元の記載: planning）
Disposition: normal
Parent: [WS131](../ws.md)、計画の正本 [design.md](../design.md)
Queue: q650（2026-10-03、Q1 の割り当て）
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

## 範囲 1: 状態の遷移と callback の順序（2026-10-03、P1、読み）

### zedBSD（`zedbsd/handoff-zedbsd.c`、`greeter.c`）: sessiond の 2 つの descriptor の上の行の protocol

| 時 | compositor がすること（今） | 送る行 | 受ける行 | backend に移した後 |
| --- | --- | --- | --- | --- |
| 最初に表示を取る前（`display.c:455` の `zwl_handoff_wait`） | `handed_over = 1`。greeter は `--auth-fd`、session は `--control-fd` に書き、GO を最長 20 s 待つ（1 byte ずつ、GO の後は読まない） | `READY` | `GO` | `kl_backend_session_ready`（同期の待ちは今のまま。他の行を読まないことも保つ） |
| login screen の Log In（`greeter_submit`） | `greeter_waiting = 1` | `AUTH name password` | `OK`（→「Starting session...」）、`FAIL`、`ERROR` | `kl_backend_session_authenticate`。答えは `host->session_answer(KL_BACKEND_SESSION_AUTH, …)` |
| lock 画面の Unlock（`greeter_submit`、`server->locked`） | `control_fd` に書く | `UNLOCK password` | `OK`（解錠）、`FAIL` | `kl_backend_session_unlock`。答えは `session_answer(…UNLOCK…)` |
| Shut Down・Restart | p005 で backend の power | `POWER poweroff｜reboot` | `OK` | 答えを `session_answer(…POWER…)` に（p005 では greeter が読んでいた） |
| greeter の auth の答えの読み（`zwl_greeter_tick`） | 行に切って `greeter_answered`。0 byte（sessiond が閉じた）なら `zwl_handoff_release` と停止 | — | 上の答え、EOF | backend の poll（auth の descriptor）で読み、行ごとに `session_answer`、EOF は `host->session_stop(KL_BACKEND_STOP_CLOSED)` |
| session の Log Out（`home.c:1491` の `zwl_handoff_logout`） | 一度だけ。clipboard の履歴を消し `logout_ms`。30 s の間に QUIT が無ければ停止 | `LOGOUT` | `QUIT` | `kl_backend_session_logout`。期限は backend の tick。clipboard の消去は compositor に残す |
| session の control の読み（`zwl_handoff_tick`） | `QUIT` → `zwl_handoff_release`・停止。lock 中の他の行 → `zwl_lock_answer`。EOF → descriptor を閉じて session は続く | — | `QUIT`、`OK`・`FAIL`、EOF | backend の poll で読む。`QUIT` は `session_stop(QUIT)`、lock の答えは `session_answer(UNLOCK…)` |
| 表示を返す（`zwl_handoff_release`） | まず出力（swapchain と lease）を閉じ、その後で書く | `RELEASED` | — | 出力を閉じるのは compositor、その後に `kl_backend_session_released` |

順序の約束: RELEASED は出力を閉じた後。QUIT・EOF の停止は callback の中で compositor が出力を閉じてから `kl_backend_session_released` を呼ぶ（callback の中から backend を呼ばない約束（design.md §3.4）があるので、compositor は flag を立てて event loop の次の回に行う）。

### Linux（`linux/seat-logind-linux.c`・`seat-direct-linux.c`・`dbus-linux.*`・`os-linux.c`）

| event | 今の処理の順 | compositor の内部に触れる所 |
| --- | --- | --- |
| open（logind） | D-Bus の接続 → `GetNameOwner` で logind の unique 名 → `GetSession` の path → signal の match → `TakeControl` → primary（`KEILAND_DRM_DEVICE`、既定 card0）の `TakeDevice`、inactive なら pause のまま | `server->os_paused` |
| open（直） | `/dev/dri/card0` を root で開く | — |
| VT（`os-linux.c`） | stdin が VT なら `KDSETMODE KD_GRAPHICS`・`KDSKBMODE K_OFF`、close で戻す | —（socket の path の決定も同じ file にある: `server->socket_path`） |
| PauseDevice（primary） | `seat_paused = 1`・`os_paused = 1` → `zwl_compose_quiesce` → `zwl_compose_output_close` → `windowed = 0` → `pause` の時だけ `PauseDeviceComplete` | 出力の停止、`os_paused`・`windowed` |
| PauseDevice（入力） | その入力の record の fd を -1（lease は残す）、`pause` なら Complete、`gone` なら閉じる（`zwl_input_close`） | `server->inputs[]` を path で探す |
| ResumeDevice（primary） | 新しい fd に差し替え → `os_paused = 0`・`windowed = 0`・`dirty = 1`（次の frame で表示を開き直す） | 同上 |
| ResumeDevice（入力） | 新しい fd（O_NONBLOCK、`EVIOCSCLOCKID` を MONOTONIC）に差し替え、record の fd・`frame_count = 0`・`discarding = 0` | `server->inputs[]` |
| 入力の read の失敗（revoke が signal より先） | lease を残し record の fd を -1（`zwl_linux_device_revoked`） | 同上 |
| bus の切断 | `server->failed = 1` | — |

### FreeBSD（`freebsd/seat-freebsd.c`・`os-freebsd.c`、libseat・seatd）

| event | 今の処理の順 | compositor の内部に触れる所 |
| --- | --- | --- |
| open | `LIBSEAT_BACKEND=seatd` → `libseat_open_seat` → enable を最長 2 s 待つ → primary の lease | `os_paused` |
| disable_seat | `os_paused = 1` → `zwl_compose_quiesce`・`output_close`・`windowed = 0` → 全ての入力を `zwl_input_close`（seatd は再開で開き直しを要る）→ 全ての lease を閉じる → `libseat_disable_seat` | 出力、`inputs[]` |
| enable_seat | primary を開き直す → `os_paused = 0`・`input_scan_time = 0`・`windowed = 0`・`dirty = 1` | 同上 |

### 移した後の host の callback（design.md §3.3 の案に、読みで分かった物を足す）

- `session_paused(data)`: 出力を止めて閉じ、`windowed = 0`（compositor）。Linux の primary の Pause、FreeBSD の disable。backend は callback から戻った後に `PauseDeviceComplete`・`libseat_disable_seat` を送る（compositor が使いを止めた後に承認する順を保つ）。
- `session_resumed(data)`: `os_paused = 0`・`windowed = 0`・`dirty = 1`（FreeBSD は `input_scan_time = 0` も）。
- `input_revoked(data, descriptor)`: その descriptor の入力の record の fd を -1（lease は backend が残す）。`input_resumed(data, old, new)`: record の fd の差し替えと `frame_count`・`discarding` の reset。`input_gone(data, descriptor)`: record を閉じる。入力の record を path で探す今の code は、backend が自分の lease の表に descriptor を持つ形に変える（backend は `server->inputs[]` を見ない）。
- `session_stop(data, reason)`・`session_answer(data, request, error)`: zedBSD の QUIT・EOF と AUTH・UNLOCK・POWER の答え。

### 分け方の案（Q1 に確認）

p006 は 3 OS の seat と session で大きく、Linux の logind の pause・resume・VT・Reboot は Linux の guest でしか確かめられない（今は「Linux の build は native、guest は使わない」）。次の 3 段に分けて commit する案:
1. p006a zedBSD の session（handoff・greeter の AUTH・UNLOCK・答えの読み、sessiond の無い session の `session/handoff-session.c`）。QEMU の p101〜p104・C1 で確かめられる。
2. p006b Linux の seat（logind・直・D-Bus・VT）と logind の電源。build と host の試験（`dbus-wire`・`seat-fd.c` の fixture）だけでは pause・resume を確かめられない。
3. p006c FreeBSD の seat（書くだけ）。
