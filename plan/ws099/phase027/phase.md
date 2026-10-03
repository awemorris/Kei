<!-- awesome-plan project=zedbsd record=ws099-p027 -->
# ws099-p027: BUG-095 — oneshot の service から `/sbin/poweroff` を呼ぶと init と待ち合い、電源が切れない

Status: in-progress（q645、P2、2026-10-03。実装済み・T1 の試験待ち）
Disposition: normal
Parent: [WS099](../ws.md)
Bug: [BUG-095](../../bugs/BUG-095.md)

## 範囲（Q1 の q645「QEMU で再現できるかから」）

BUG-095 の原因（ticket のコードの読み、ws075-p015 の 5330 の gdbstub）を init の側で直し、QEMU で再現と直しを確かめる。kernel の
「受け付け前の unix stream の client で `poll` が準備ありを返す」（client の待ちが CPU を回す件）は範囲の外（別の調査）。

## 原因（コードの読み）

`userland/base/init/main.c` の `spawn_service` は `type=oneshot` の service の終わりを `waitpid(child, &status, 0)` で待つ。その間 init は
main の loop の `accept4` に戻らず、control socket の要求に答えない。oneshot の中の `/sbin/poweroff` は init の答えを
`ZSV1_CLIENT_TIMEOUT_SECONDS`（310 秒）まで待ち、init はその oneshot を待つので、両方が待ち合い、要求は失われる。capture の image の
`poweroff` の service（`plan/ws031/tests/zdesktop/poweroff`、`after=vkwait2`）がこの形。QEMU でも同じ（init のコードは machine に依らない）。

## 直し（`userland/base/init/main.c`）

- control socket の listener を static（`control_listener`）にし、oneshot の待ち（`wait_for_oneshot`）は `waitpid(WNOHANG)` と
  listener の `poll`（100 ms）を回し、その間も要求を受ける（`serve_while_oneshot`）。
  - halt・poweroff・reboot と、読むだけの要求（list・show・reload）はすぐ答える。
  - service を起こす・止める要求（start・stop・restart）は、今までどおり init の loop で行う: 8 件まで保持し（`deferred_requests`）、
    loop の先頭の `handle_deferred_requests` で古い順に行う。9 件目は `EBUSY oneshot-running` で断る。
- system の action（halt・poweroff・reboot。control socket でも signal でも）が求められたら oneshot の待ちを終える（service は pid を
  持ったまま `shutdown_system` の `stop_service` で止まる）。起動の途中（`start_enabled_services`）なら残りの service を起こさない。
- `handle_request` を受信（`refuse_request`）と実行（`dispatch_request`）に分けた。要求の扱い自体は変えていない。

## 試験（QEMU は T1）

- `plan/ws099/tests/bug095/`: SSH の guest の image（`config-amd64-ssh.mk`）に oneshot の service `bug095_poweroff`（起動の 30 秒後に
  `/sbin/poweroff`）を足す `build-bug095-image.sh BUILD`、guest を起こして QEMU の process が LIMIT 秒（既定 180）以内に終わるかを見る
  `bug095-test.sh IMAGE [LIMIT]`（判定は emulator の process の終わりだけ、console は読まない）。
- 期待: 直しの前の init の image で FAIL（再現、emulator が 180 秒で終わらない）、直しの後で PASS。

## 確かめ

- build: `make ZEDBSD_CONFIG=plan/ws099/tests/config-amd64-criteria.mk BUILD=build/p2-p024-img build/p2-p024-img/bin/init`（-Werror）exit 0、
  warning 0。`plan/tools/style-check.py` は変えた行に違反 0（file の既存の違反は残る）。
- host で init は動かせない（PID 1、zedBSD の UAPI）。QEMU の再現と直しは T1 に依頼（結果は未着）。
