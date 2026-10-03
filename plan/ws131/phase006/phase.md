<!-- awesome-plan project=zedbsd record=ws131-p006 -->

# ws131-p006: backend の seat・session の領域

Status: in-progress（q650、2026-10-03、P1。範囲 1 の表、p006a（zedBSD の session）・p006b（Linux の seat と電源）・p006c（FreeBSD の seat、書くだけ）の実装と host の確認は済み。QEMU（zedBSD）と Linux の guest（QEMU+KVM）の確認は試験の担当待ち。元の記載: planning）
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

## p006a: zedBSD の session（2026-10-03、P1）

Q1（2026-10-03）: 3 段の分け方は OK、p006a を進める。p006b は user の判断（(A) Linux の guest を使う／(B) build だけ）待ちで入らない。

### 変えたこと

- `libkeiland-backend/keiland-backend.h`:
  - session の領域: `KL_BACKEND_SESSION_NONE`・`AUTH`・`UNLOCK`・`POWER`（答えの request）、`KL_BACKEND_SESSION_QUIT`・`ENDED`・`UNANSWERED`（停止の理由）。
  - 関数: `kl_backend_session_ready`・`logout`・`authenticate`・`unlock`・`managed`。
  - host の `session_stop(reason)`・`session_answer(request, error)`、options の `session_descriptor`。
  - 答えの error: OK → 0、FAIL → EACCES、ERROR → EIO、他の行 → EPROTO。
  - design.md §3.3 の `kl_backend_session_released` は置かなかった。QUIT・ENDED の時は compositor が callback の中で出力を閉じ、callback から戻った後に backend が `RELEASED` を書く。こうすると「出力を閉じた後に RELEASED」の順と、callback の中で backend を呼ばない約束（§3.4）を両方守れる。
- `git mv wayland/zedbsd/handoff-zedbsd.c → libkeiland-backend-zedbsd/session-zedbsd.c` で書き直した。
  - READY・GO の同期の待ち（20 s、GO の後は読まない）。
  - AUTH・UNLOCK（一度に一つ、他は EBUSY、password は送った後に消す）、LOGOUT（一度だけ、30 s の期限は tick）。
  - 2 つの descriptor の行を tick で読む。login screen の descriptor の EOF は ENDED。session の descriptor の EOF は、閉じて session は続く（managed 0）。
  - power の OK は `session_answer(POWER)`（power-zedbsd.c が request を記録する）。
- `git mv wayland/session/handoff-session.c → libkeiland-backend/session/session-none.c`（Linux・FreeBSD）: 全て ENOTSUP、managed 0。
- `backend.c`: `kl_backend_tick` が各 OS の `kl_backend_session_tick` を呼ぶ。`backend-private.h` に session の状態。
- compositor:
  - 新規 `wayland/handoff.c`（共通、3 OS）: `zwl_handoff_wait`・`logout`・`tick` を backend の上に、callback の `zwl_handoff_stop`・`zwl_handoff_answer`。
  - log は今の行のまま: `ZWL HANDOFF go=`・`logout`・`quit`・`released`・`logout unanswered`、`ZWL GREETER closed`。
  - `greeter.c`: auth の descriptor を読まない。AUTH・UNLOCK は backend、答えは `zwl_greeter_answer(request, error)`。`ZWL GREETER answer=OK|FAIL|ERROR` の log は同じ（認識しない行は `?`）。
  - `zwl_lock`・App Home の Lock Screen の有無は `kl_backend_session_managed`。
  - `shell.c` の login screen の tick で `zwl_handoff_tick` も呼ぶ。
  - `main.c` は 2 つの descriptor と callback を backend に渡す。
- build: `sources.mk` に session-zedbsd.c。Linux・FreeBSD の backend に session-none.c。compositor の 3 つの Makefile は handoff-zedbsd.c・handoff-session.c を handoff.c に替えた。

### 確認（host）

| 確認 | 結果 |
| --- | --- |
| `plan/ws131/tests/host-session.sh`（新規、ASan・UBSan、socketpair を sessiond の代わりに） | 31/31 |
| `host-session.sh` の中身 | READY・GO、AUTH の行、EBUSY、名前の空白の EINVAL、FAIL → EACCES、2 回に分けて届く ERROR → EIO、POWER の OK → request POWER、頼んでいない行 → NONE・EPROTO、login screen の shutdown → ENDED と、stop の後の RELEASED、UNLOCK と OK、LOGOUT は一度、QUIT → QUIT と stop の後の RELEASED、sessiond が閉じた後は unmanaged と logout の ENOTSUP、30 s の期限（34999 では無し、35000 で UNANSWERED） |
| `host-power.sh` | 17/17・6/6 |
| zedBSD の build（libkeiland.so・wayland） | exit 0、warning 0 |
| Linux の build（native の gcc・clang） | exit 0、warning 0。header-check PASS（348）、B1 の未定義の symbol 0 |
| `check.sh` | PASS |
| makefile-sync | 既存の誤検出（apps.c）だけ |
| FreeBSD | 書くだけ |

### QEMU の試験（試験の担当に予約、結果待ち）

- boot-test。
- login の image（`build-login-image.sh BUILD graphical`）で `zdesktop-p095.sh`（間違った password の FAIL と login）、`zdesktop-p101.sh`（READY・GO・RELEASED の受け渡し）、`zdesktop-p102.sh`（lock と unlock）。
- `graphical-network` の image で `zdesktop-p104.sh`。
- `zdesktop-p103.sh`。
- criteria の image で `c1-boot-shutdown.sh`（Log Out → QUIT → greeter → Shut Down）。

## p006c: FreeBSD の seat（書くだけ、2026-10-03、P1）

Q1（2026-10-03）: p006b の返事を待つ間、p006c の FreeBSD の書くだけを先にしてよい。FreeBSD の build・audit・guest は user の指示で行わない。

### seat の interface（Linux の p006b も同じ物を使う）

- `keiland-backend.h`:
  - seat の関数: `kl_backend_seat_open`・`close`・`primary_fd`・`primary_path`・`paused`・`device_open`・`device_close`・`device_revoked`。
  - host の callback: `session_paused`・`session_resumed`・`input_paused(path)`・`input_resumed(path, descriptor)`・`input_gone(path)`、停止の理由 `KL_BACKEND_SESSION_LOST`。
  - 入力は device の path で識別する。descriptor は pause・resume で変わるが、path は変わらない。backend は compositor の `inputs[]` を見ない。
- `backend.c` の `kl_backend_poll_*` は、各 OS の `kl_backend_seat_poll_*`（`backend-private.h`）へ渡す。
- `libkeiland-backend/unsupported/seat-unsupported.c`（新規）: zedBSD（compositor は自分の kernel の interface を使う）と、p006b までの Linux。open は ENOTSUP、poll は 0。

### FreeBSD

- `git mv wayland/freebsd/seat-freebsd.c → libkeiland-backend-freebsd/seat-freebsd.c` で書き直した。
  - `seat-freebsd.h` は削除した。
  - seatd の選択（`KEILAND_SEAT`）と primary の path の検証は backend の open に入れた。最初の activation は open の中で待ち、compositor には callback で知らせない（compositor は open の後に `kl_backend_seat_paused` を読む）。
  - lease は path を持つ。
  - disable の順: `session_paused`（compositor が描画を止め出力を閉じる）→ 各入力の `input_gone(path)` と lease の close → primary の lease → `libseat_disable_seat`。
  - enable の順: primary を開き直す → `session_resumed`。
  - libseat の callback の中の失敗と、service の HUP は、dispatch の後に `session_stop(LOST)`。
  - 閉じる時は callback を呼ばない（compositor は終わる所）。
- compositor:
  - `freebsd/os-freebsd.c` は backend の seat と poll を使う。表示の acquire は p008 まで compositor に残し、primary の fd は backend から取る。
  - 新規 `freebsd/input-seat-freebsd.c`: evdev の `zwl_seat_device_*` を backend へ。
  - 新規 `wayland/backend-host.c`（共通）: 5 つの callback。paused は quiesce・output close・`windowed = 0`。resumed は `os_paused = 0`・`input_scan_time = 0`・`windowed = 0`・`dirty = 1`。input は path で探す。
  - `input.c` に `zwl_input_forget`（seat が閉じた descriptor を返さずに record を消す）。
  - `handoff.c` は LOST で `failed = 1`。
  - `evdev/seat.h` の `zwl_seat_paused` は server を取る（Linux の adapter は使わない）。
- build: backend の `Makefile.freebsd` に seat-freebsd.c と、libseat.h の生成を待つ規則（compositor の Makefile.freebsd から移した）。compositor の一覧は `input-seat-freebsd.c`・`backend-host.c`。

### 確認（host だけ、FreeBSD の build は未実施）

| 確認 | 結果 |
| --- | --- |
| `plan/ws131/tests/host-seat-freebsd.sh`（新規。seat-freebsd.c を host で、作り物の libseat（`tests/fake-libseat/libseat.h` と test の中の関数）と） | 13/13。最初の activation は resume にならない。disable の順は `paused gone:/dev/zero close close disable`。disable 中の入力は EAGAIN。enable は `open:/dev/null resumed`。HUP は `stop:4`（LOST）。close で全て返る |
| FreeBSD の compositor の file（os-freebsd.c・input-seat-freebsd.c）と backend-host.c・handoff.c・input.c | Linux の header と作り物の libseat.h で gcc の `-fsyntax-only` が通る（FreeBSD の build の代わりではない） |
| zedBSD の build（libkeiland.so・wayland） | exit 0、warning 0 |
| Linux の build（native の gcc・clang） | exit 0、warning 0。header-check PASS（350）、B1 は 0 |
| `check.sh` | PASS |
| `host-session.sh`・`host-power.sh` | 31/31、17/17・6/6 |
| FreeBSD の native build・`native-build-audit.py`・起動 | 未実施（user の再開の指示待ち） |

## 判断（2026-10-03 ユーザー）

p006b の確認の方法: (A)。ユーザー「なるほど、LinuxでのテストはQEMU+KVMを使ってください。」→ p006b の logind と直の seat での起動、VT の切り替えの後の復帰、使い捨ての guest の Reboot は Debian の QEMU+KVM guest で T1・T2 が確かめる（AGENTS.md の検証の例外に記録）。

## p006b: Linux の seat と logind の電源（2026-10-03、P1）

Q1（2026-10-03）: 書く code は (A)/(B) で同じなので実装に入る。その後 user の判断は (A)（「なるほど、LinuxでのテストはQEMU+KVMを使ってください。」、Q1 が AGENTS.md の検証の例外に記録）。Linux の guest の確認は試験の担当に依頼する。

### 変えたこと

- `git mv` で `wayland/linux/` → `libkeiland-backend-linux/` へ移した:
  - `dbus-linux.c`・`dbus-linux.h`（変更は header guard と冒頭の 1 行だけ）
  - `seat-logind-linux.c`・`seat-direct-linux.c`・`seat-linux.h`（書き直し）
- `seat-logind-linux.c`: compositor の内部（`os_paused`・`zwl_compose_*`・`windowed`・`dirty`・`inputs[]`・`zwl_input_close`）を触らず、callback を使う。
  - primary の pause: `session_paused` → その後に `PauseDeviceComplete`（cooperative の時）。
  - 入力の pause: `input_paused(path)`。gone: `input_gone(path)` → lease の返却。primary の gone: ENODEV → `session_stop(LOST)`。
  - resume: 新しい fd に差し替え、`EVIOCSCLOCKID` → `session_resumed`、または `input_resumed(path, fd)`。
  - `<linux/input.h>` を直に include する（前は compositor の zwl-evdev.h から）。
- 新規 `seat-linux.c`: seat の選び方（`KEILAND_SEAT`、無ければ Wayland の session の ID で logind、他は direct、未知は EINVAL）、VT（`KDSETMODE`・`KDSKBMODE` と close での復元、もと `wayland/linux/os-linux.c`）、`kl_backend_seat_*` と poll（logind の bus、dispatch の失敗と HUP は `session_stop(LOST)`）。
- 新規 `power-linux.c`: logind の `CanPowerOff`・`CanReboot`・`CanSuspend`（"yes"・"challenge" で offered）と、`PowerOff`・`Reboot`・`Suspend`（interactive=false）。
  - 呼ぶたびに system bus を開いて閉じるので、direct の seat でも使える。
  - Linux では logind が呼び出しに答えるので、action の戻り値が答えになる（`session_answer` は無い、header に書いた）。
- compositor:
  - `wayland/linux/os-linux.c` は socket の path、`KEILAND_DRM_DEVICE` の setenv、表示の acquire（p008 まで）、poll の委譲だけになった。
  - `linux/input-seat-linux.c` と `freebsd/input-seat-freebsd.c` を共通の `evdev/seat-backend.c` 一つにした。
  - logind が revoke の後に残す入力（`kl_backend_seat_device_revoked` が 1）は、この adapter が自分の record の fd を -1 にする（ws105-p009 の補正を compositor の側に保つ）。
- build: backend の `Makefile.linux` から unsupported の seat と power を外し、dbus・seat-linux・seat-logind・seat-direct・power-linux を足した。compositor の `Makefile.linux` は os-linux.c と seat-backend.c。
- 道具: `plan/tools/keiland-linux/dbus-wire.c` と README の dbus-linux の path（委任の範囲の path だけ）。checker の L2 の古い `libkeiland/linux` の path を外した。

### 確認（host）

| 確認 | 結果 |
| --- | --- |
| Linux の build（native の gcc・clang） | exit 0、warning 0。header-check PASS（350）、install、elf-check PASS（24 ELF）。`libkeiland-backend.a` の未定義の symbol に `zwl_`・`kwl_`・`keiland_` が無い（B1）。compositor は `kl_backend_seat_*`・`kl_backend_power_*` を 13 個持つ |
| zedBSD の build（libkeiland.so・wayland） | exit 0、warning 0 |
| `dbus-wire`（README の fixture、新しい path） | 普通と ASan・UBSan の両方で `dbus-wire: ALL PASS`（5 ケース） |
| `seat-fd.c` | build できる（実行は guest、README の通り） |
| `plan/ws131/tests/host-seat-linux.sh`（新規） | 12/12。未知の seat と相対の primary の EINVAL、direct の seat（/dev/null）の open、pause 無し、poll 0、入力の open、revoke で残さない、close で primary を返す、power の未知の action の ENOTSUP と backend 無しの EINVAL、Linux に session manager が無いこと |
| `check.sh` | PASS |
| `host-session`・`host-power`・`host-seat-freebsd` | 31/31、17/17・6/6、13/13 |
| logind の pause・resume・gone の callback の順 | host では未実施（session が要る）。Linux の guest で確かめる |

### Linux の guest の確認（user の判断 (A)、試験の担当に依頼、結果待ち）

ws105-p009（q536）の手順で、gdm の guest（`build-guest.sh … gdm`、`guest.sh gdm-setup`）を使う。
- logind の seat で Keiland の session が起動する。
- `SwitchTo` と `chvt` の VT の切り替えの後に、画面と入力が戻る。
- base の guest で root の direct の seat が起動する。
- 使い捨ての guest で logind の Reboot（`busctl call … Reboot b false`）の後に再起動する（SSH が切れて再びつながる）。power-linux.c と同じ logind の呼び出しを外から確かめる形。

## p006b の結果（Q1、2026-10-03、T1-042、Debian 13 の QEMU+KVM）

試験 1 logind の起動 PASS、試験 2 VT の切り替え 2 通り PASS（pid 不変、paused/resumed、復帰後の click で App Home）、試験 4 logind の Reboot PASS（boot_id が変わる）。
試験 3 direct の seat: `wayland --timeout=15`（--glass・--session 無し）で画面は背景色 (32,48,64) の 1 色、入力 4 個を direct で開き、15 s で終わり chvt 1 で tty1 に戻る。P1 の判定: この起動の仕方では期待どおり（glass の look を出さない）、依頼の command の不足で code の問題ではない。p006b は確認済みとする。
p006 全体の判定は p006a の T2-007 の結果を待つ。
