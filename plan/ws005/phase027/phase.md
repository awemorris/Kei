<!-- awesome-plan project=zedbsd record=ws005-p027 -->
# ws005-p027: BUG-151 — TCP の自分側の shutdown(SHUT_WR) で poll が POLLHUP を返す不具合を kernel で直す

Status: in-progress（実装済み・T1 の試験待ち）
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
