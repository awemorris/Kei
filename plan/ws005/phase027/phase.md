<!-- awesome-plan project=zedbsd record=ws005-p027 -->
# ws005-p027: BUG-151 — TCP の自分側の shutdown(SHUT_WR) で poll が POLLHUP を返す不具合を kernel で直す

Status: cleared（Q1 判定 2026-10-03: T1-025（QEMU）bug149-poll pass=12 fail=0（tcp-shutwr-answer read=3 text=ans、tcp-peer-fin-hangs-up read=0）、A2 PASS、net/service exit 0）。元の記載: uncleared（T1-017 FAIL）→ 修正 2 を実装・T1 の再試験待ち
Disposition: normal
Parent: [WS005](../ws.md)
Bug: [BUG-151](../../bugs/BUG-151.md)
Queue: q641（P1 generation11、2026-10-03。Q1 の dispatch。承認: user「実機がなくても修正できるバグをP1で修正、T1で順次テスト、のパイプラインを実行してください。」「実行してください。」）
依存: ws005-p025（BUG-149、同じ形の AF_UNIX の修正、cleared）

## 範囲

`src/kern/net/tcp.c` の `tcp_poll` から「自分側の `write_shutdown` だけで POLLHUP」を除く（BUG-149 と同じ形）。自分側の SHUT_WR は POLLOUT を
出さないだけにする（従来どおり `!socket->write_shutdown` の条件）。変えないもの: `socket->error` の POLLERR、`read_shutdown`（相手の FIN・RST・自分の SHUT_RD）・
CLOSE_WAIT・TIME_WAIT・close の途中（`lifecycle != SOCKET_OPEN`）の POLLHUP、listener の POLLIN、SYN_SENT の error の POLLOUT。

## 試験

[bug149-poll.c](../tests/bug149-poll.c) の TCP の INFO を判定のある 3 case にした: `tcp-shutwr-nothing-yet`（write → SHUT_WR → poll(POLLIN, 0) が 0）、
`tcp-shutwr-answer`（server の答えが読める）、`tcp-peer-fin-hangs-up`（server の FIN の後 read 0 と POLLHUP）。Linux の host で 3 case とも PASS（基準の確認。
AF_UNIX・UDP の他の case は Linux では違いがあり、zedBSD の guest で見る物）。修正後の zedBSD の期待は RESULT pass=12 fail=0。

## 検証

- build: `make -j8 ZEDBSD_CONFIG=plan/ws089/tests/config-amd64-settings.mk BUILD=build/p1-q640 build/p1-q640/vmunix` → exit 0、warning 0、`amd64 vmunix check: PASS`。
- probe の build: `sh plan/ws005/tests/bug149-build.sh build/p1-q641-probes` → 2 本できた。
- host: 上の Linux の基準。kernel の tcp_poll の host 試験は無い（読みで確かめた）。
- QEMU（T1 に依頼）: lean image（`plan/ws001/tests/config-amd64-lean-guest.mk` + bug149 の probe）で `bug149-check.sh`。未実施（結果待ち）。
- 実機は未実施（TCP の SHUT_WR を使う zedBSD の userland は無い）。

## T1-017（2026-10-03、QEMU）: FAIL と修正 2

`tcp-shutwr-nothing-yet` は PASS（修正 1 は効いた）。`tcp-shutwr-answer` が `ready=1 revents=0x11 read=0`（答えの前に EOF）で FAIL し、続く
`tcp-peer-fin-hangs-up` の read が戻らず probe が止まった（2 回とも同じ）。読みで原因を特定した:

1. **`tcp_sendto` が ESTABLISHED の時しか送らなかった。** client の FIN を受けた server は CLOSE_WAIT になり、答えの write が ENOTCONN で捨てられ、
   server の FIN だけが届いて client は EOF を読んだ（半分閉じた接続で相手が答えられない。BUG-151 の場面そのもの）。RFC 793 のとおり CLOSE_WAIT でも
   送れるようにした。poll の POLLOUT も CLOSE_WAIT を含める。
2. **EOF の印を一度読んだ後の read が永久に待った。** 相手の FIN は空の packet（EOF の印）として queue に入るだけで、それを読んだ後、LAST_ACK・
   TIME_WAIT（と FIN の後の CLOSED）の read は data を待って眠った。queue が空でこれらの状態なら 0 を返すようにした（RST の後の CLOSED は error を返す
   従来どおり）。
3. 試験: TCP の read を `recv(MSG_DONTWAIT)` にし（kernel の不具合で probe が止まらない）、server の write の戻り値（sent=3）と、EOF の後の 2 度目の
   read も 0 であることを見る。Linux の host で 3 case とも PASS。

build: vmunix warning 0、`amd64 vmunix check: PASS`、probe の build OK。QEMU の再試験は T1 に依頼（結果待ち）。
