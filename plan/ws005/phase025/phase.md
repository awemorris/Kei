<!-- awesome-plan project=zedbsd record=ws005-p025 -->
# ws005-p025: BUG-149 — AF_UNIX の自分側の shutdown(SHUT_WR) で poll が POLLERR を返す不具合を kernel で直す

Status: cleared（Q1 判定 2026-10-03、統合 cb3da62d1。元の記載: in-progress（q635-i01、P1 generation10、2026-10-03。P1 は受け入れ A1〜A5 を満たしたので cleared を提案、判定は Q1）
Disposition: normal
Parent: [WS005](../ws.md)
Bug: [BUG-149](../../bugs/BUG-149.md)
Queue: q635（承認: user 2026-10-03「P1の試験が終了してclearedになったら、BUG-149をP1で取り組んでもらってください。」＋「いったんclearedにして先に進めましょう。」。時限 4h）

## 目的

request を書いて `shutdown(SHUT_WR)` し、`poll(POLLIN)` で答えを待つ client（libkeiland の network、`zsv1-client`、sessiond の greeter の release、`net` の CLI）が、
答えの前に poll から戻されないようにする。Linux・FreeBSD と同じく、自分側の write shutdown は POLLOUT を出さないだけで、error（POLLERR）ではない。

## 範囲

1. `src/kern/net/unix-socket.c` の stream の poll（`unix_poll`）と `src/kern/net/socket.c` の汎用の poll（`socket_poll_common`。route socket・unix の datagram と
   listener・ops に poll の無い socket が使う）から「自分側の `write_shutdown` だけで POLLERR」を除く。POLLOUT を出さないことは保つ。
   - 従来どおり残すもの: socket の close の途中（`lifecycle != SOCKET_OPEN`）の POLLERR・POLLHUP、`socket->error`（SO_ERROR）の POLLERR、
     `read_shutdown`（相手の SHUT_WR・相手の close）の POLLIN・POLLHUP、相手が読まなくなった・相手が居ない書き手の POLLERR・POLLHUP。
   - TCP（`tcp.c` の `tcp_poll`）は所有外。自分側の write shutdown で POLLHUP を出す同じ形の挙動は、直さずに報告する（Q1 の指示 2026-10-03）。
2. 試験（guest、lean の amd64 image、serial の対話）: [bug149-poll.c](../tests/bug149-poll.c)（stream・nonblocking・datagram・UDP の SHUT_WR の後の poll、
   相手の close の POLLHUP、SHUT_RDWR、相手の SHUT_RD の POLLERR、TCP の拒否の SO_ERROR と POLLERR、TCP の SHUT_WR の観察）と
   [bug149-keiland.c](../tests/bug149-keiland.c)（実物の libkeiland の `network-zedbsd.c` を、3 秒後に ENOENT を答える代わりの daemon に向けた join）。
   build は [bug149-build.sh](../tests/bug149-build.sh)、実行は [bug149-check.sh](../tests/bug149-check.sh)。直す前に FAIL、直した後に PASS を示す。
3. BUG-149 の (a)〜(d) の確認: (a) libkeiland の join の EIO が kernel の修正だけで直るか（上の試験、鍵は使わない）、(b) `zsv1-client` の busy loop
   （読みと nonblocking の試験、`time service ...`）、(c) sessiond の greeter の release の上限（読み）、(d) `net` の CLI の回帰（guest で `net show` ほか）。
4. P3 の ws131-p003 の libkeiland の回避（`recv(MSG_PEEK|MSG_DONTWAIT)`）の要否の見解（P3 の file は変えない）。

範囲外: userland（libkeiland・zsv1-client・sessiond・net）の修正（要ると分かったら案を Q1 に送る）、TCP の poll、WiFi の鍵を使う確認（WS133）。

## 受け入れ

- A1: 修正前の kernel で `bug149-poll` の SHUT_WR の case（stream・nonblocking・datagram・UDP・SHUT_RDWR）が FAIL し、修正後に全 case が PASS（相手の close の
  POLLHUP、相手の SHUT_RD の POLLERR、TCP の拒否の POLLERR を含む）。
- A2: 修正後の kernel で `bug149-keiland` が PASS（join が 3 秒の後に daemon の ENOENT で終わる）。修正前の結果も記録する。`network-zedbsd.c` は
  P1 の worktree の版（main 8bd922baf、P3 の回避 `43c9fb90c` を含まない）を link するので、PASS は「kernel の修正だけで直る」証拠になる（Q1 2026-10-03）。
- A3: `net` の CLI・`service` が修正後も通る（exit 0、出力の形が変わらない）。
- A4: 変えた kernel の build（lean image）が warning 0、`plan/tools/boot-test.sh` で login prompt（PNG）。
- A5: (a)〜(d) の結論と P3 の回避の要否を BUG-149 と下の結果に書く。

## 所有 path

`src/kern/net/unix-socket.c`、`src/kern/net/socket.c`、`plan/ws005/phase025/`、`plan/ws005/tests/`（bug149-*）、`plan/ws005/ws.md`（Phase の表の行）、`plan/bugs/BUG-149.md`。

## 依存

なし（kernel の修正は P3 の ws131-p003 と独立。P3 の file は変えない）。

## 検証の上限

QEMU の guest は同時に一つ。同じ条件で変更なしの retry は 3 回まで。console/serial の log の file は読まず、serial.py の対話の command の出力と終了状態を証拠とする。
集約の `make check` は走らせない。

## q635-i01 の結果（P1 generation10、2026-10-03、base main `8bd922baf`、commit `9915fbd55` と次の commit）

### 修正

- `src/kern/net/unix-socket.c` の `unix_poll`（stream）: POLLERR は `socket->error` と `lifecycle != SOCKET_OPEN` の時だけ。自分側の `write_shutdown` は `local_writable` を偽にして
  POLLOUT を出さないだけ（従来どおり）。
- `src/kern/net/socket.c` の `socket_poll_common`: `else if (write_shutdown) POLLERR` を `lifecycle != SOCKET_OPEN` の時の POLLERR に替えた（close の途中の socket は
  close が `write_shutdown` も立てるので、変更前と同じく POLLERR）。使うのは route socket・unix の datagram と listener・poll の op の無い socket。
- 変えないもの: 相手の close・相手の SHUT_WR（自分の `read_shutdown`）の POLLIN・POLLHUP、相手が居ない・相手が SHUT_RD の書き手の POLLERR|POLLHUP、`socket->error` の POLLERR。
  UDP は `shutdown` 自体が EOPNOTSUPP（errno 21）で影響なし。TCP は `tcp_poll` で別（下の関連 bug）。

### 確認（すべて QEMU、lean の amd64 image `BUILD=build/p1-net`・`plan/ws001/tests/config-amd64-lean-guest.mk`、KVM。serial.py の対話の command の出力と終了状態。console の log は読んでいない）

| 確認 | 修正前（`build/p1-bug149/pre.img`） | 修正後（`post.img`・`post2.img`） |
| --- | --- | --- |
| A1 `bug149-poll` | pass=3 fail=6: SHUT_WR の直後 revents=POLLERR、答え待ちが elapsed=0ms、nonblocking が 0ms で戻る、SHUT_RDWR が POLLERR 付き、unix dgram の SHUT_WR が POLLERR | pass=9 fail=0: 答えまで 300ms 待つ、nonblocking は 500ms の timeout まで眠る、相手の close で POLLIN|POLLHUP（POLLERR 無し）、相手の close（SHUT_WR 無し）POLLIN|POLLERR|POLLHUP、相手の SHUT_RD で POLLERR|POLLHUP、TCP の拒否で POLLERR と SO_ERROR=ECONNREFUSED |
| A2 `bug149-keiland`（worktree の `network-zedbsd.c`、P3 の回避 `43c9fb90c` を含まない） | FAIL error=EIO（network_readable が即真 → blocking read が 3 秒待つ間に 2 秒の deadline が切れる） | PASS: 3000〜3101ms 後に daemon の ENOENT（errno 6） |
| A3 `net show`・`net wifi list`・`net wifi disconnect`・`service list/status/restart` | exit 0 | exit 0、出力同形 |
| (b) `time service list` ×6・`time service status networkd` ×6 | real ≈1.00s、sys ≈0.94s（busy loop） | real ≈0.99s、sys 0.00s |
| A4 build | — | kernel の warning 0（`-Werror`、rc=0、log の `warning:` 0 件）。`plan/tools/boot-test.sh` PASS: `build/p1-bug149/boot-test/login.png` |

- `service` の real の約 1 秒は修正の前後で同じで、server（init）側の時間（この不具合と無関係）。
- 試験の file: [bug149-poll.c](../tests/bug149-poll.c)・[bug149-keiland.c](../tests/bug149-keiland.c)（style-check 0 件）、[bug149-build.sh](../tests/bug149-build.sh)・
  [bug149-check.sh](../tests/bug149-check.sh)。出力は worktree の `build/p1-bug149/{pre,post,post2}.out`。
- kernel の 2 file の style-check の件数は変更の前後で同じ（unix-socket.c 154・socket.c 62、新しい違反 0）。`git diff --check` clean。

### BUG-149 の (a)〜(d)

- (a) libkeiland の join の EIO: **kernel の修正だけで直る**（A2）。P3 の回避（poll の後の `recv(MSG_PEEK|MSG_DONTWAIT)`）は修正後の zedBSD では不要。Linux・FreeBSD でも
  自分側の SHUT_WR で POLLIN/POLLERR は立たない（Linux の `unix_poll` は `sk_err` で POLLERR、両方向の shutdown で POLLHUP。FreeBSD の `sopoll_generic` も
  受信側の終わりで POLLIN、両方向で POLLHUP）ので、他の OS の backend でも元々不要（読みの判断）。回避は害が無い（peek が 0 byte なら読めないと判定するだけ）ので、
  残すか外すかは P3／Q1 の判断。外す場合は修正後の kernel が前提。
- (b) `zsv1-client` の busy loop: kernel の修正だけで消える（上の sys の時間）。userland の修正は不要。
- (c) sessiond の greeter の release: 読みの判断。`sessiond_greeter_release` は SHUT_WR の後 `sessiond_read_line(..., GREETER_RELEASE_MS)` が poll で待つ。修正前は
  POLLERR で即返り blocking の read が上限なしで待った。修正後は poll が上限まで待つので上限が効く。QEMU の観察は未実施。userland の修正は不要。
- (d) `net` の CLI: 回帰なし（A3）。`wait_input` の deadline と SO_RCVTIMEO は同じ秒数なので、修正後も上限は同じ。

### 関連の bug（新しい症状・同じ形の別の不具合）

- **TCP の `tcp_poll`（`src/kern/net/tcp.c` 2233 行、2279〜2284 行の `socket->write_shutdown ||` → POLLHUP）**: 自分側の SHUT_WR だけで POLLHUP を出す。`bug149-poll` の
  INFO（loopback の TCP で write → SHUT_WR → poll(POLLIN, 0)）が修正の前後とも revents=POLLHUP（Linux・FreeBSD は 0）。起こる状況: TCP で request を書いて SHUT_WR し、
  poll で答えを待つ client。影響の見込み: POLLHUP を「終わり」と扱う client は答えの前に読みに行く（blocking なら read が待つ、nonblocking なら EAGAIN の busy loop、
  POLLHUP で閉じる client は答えを失う）。zedBSD の userland で TCP に SHUT_WR を使うものは見つからなかった（`grep SHUT_WR`: 使うのは AF_UNIX の 4 か所だけ）ので、
  現状の影響は外部 package（HTTP client など）に限られる見込み。P1 の所有外のため直していない（Q1 の指示）。
- ほかの新しい症状は見つからなかった。

### 残り・再開点

- 実機（5330）の確認は未実施（S1 の image で Q1/user が行う）。(c) の QEMU の観察は未実施（読みの判断）。
- P3 の回避の要否は上の見解。P3 の file は変えていない。

2026-10-03 T1-010（QEMU）: 直した bug149-build.sh（backend の source で link）で A1 pass=9 fail=0、A2 `PASS keiland-slow-join elapsed=3000ms`、net・service exit 0。
