<!-- awesome-plan project=zedbsd record=ws099-p028 -->
# ws099-p028: zdesktop の log に NUL の byte が入る（試験の grep が log を binary と読む）

Status: in-progress（実装済み・T1 の試験待ち）
Disposition: normal
Parent: [WS099](../ws.md)
Queue: q643 の続き（P1 generation11、2026-10-03。Q1 の指示: 見つけた担当がその場で直す。user「実機なしで解決できるバグをどんどん処理してください。」）
発見: T1-030（menu-bug148）。`/tmp/zdesktop.log` に NUL が入り、zedBSD の grep が行を出さず「binary file matches」とだけ返した。

## 原因（読みと host の再現）

試験は zdesktop を `> /tmp/zdesktop.log 2>&1` で起動する（O_APPEND なし）。前の zdesktop（や、それが起動した program。stdout を共有する）が終わる途中、
または生きている間に、次の run の `>` が file を 0 に切り詰めると、前の writer の次の write は自分の古い offset に書かれ、その手前が NUL の穴（sparse）
になる。host の `plan/ws099/tests/host-log-append.sh` で再現した（O_APPEND なし 66 byte の NUL、あり 0）。

## 修正（`userland/desktop/wayland/main.c`）

起動の直後に stdout・stderr の open file に `O_APPEND` を立てる（`log_append`、fcntl F_GETFL/F_SETFL）。zdesktop が起動する program は同じ open file を
共有するので、それらの write も末尾に付く。pipe・端末では効果がなく、立てられない時はそのまま。zedBSD の kernel は F_SETFL の O_APPEND を扱う
（`src/kern/syscall.c`）。試験の側の `grep -a`（56f02e9f6）はそのまま残す。

## 検証

- host: `sh plan/ws099/tests/host-log-append.sh` → `plain: NUL bytes=66`、`append: NUL bytes=0`、PASS。
- build: wayland の build は warning 0。
- QEMU（T1 に依頼）: `plan/ws099/tests/log-nul-guest.sh`（新規。zdesktop を `>` で起動し、READY の後に file を切り詰めてから止める。size と
  `tr -d '\000'` の後の size が等しく、ZWL EXIT の行が残る）。未実施（結果待ち）。修正前の image では FAIL の見込み。
