<!-- awesome-plan project=zedbsd record=ws130 -->

# WS130: IPv6 の network stack

<!-- awesome-plan-current:start -->
Status: planning
Primary Milestone: MG005
Related Milestones: MG002（POSIX の socket API）
Objectives: O1, O3
Parent: [Master](../master.md)
Queue: none
Resume point: p001（設計）。ベータ1 では計画だけ、実装はベータ2 以降（2026-10-03 user）。
<!-- awesome-plan-current:end -->

## 目標（2026-10-03 ユーザー）

「WSを立てましょう。計画だけ行っていきます。ベータ2以降で実装します。dhcpc -6というコマンドを作るのがいいと思います。SLAACはどうすればいいかなあ。NDPもどうすれば。」

- 今の kernel の network stack（`src/kern/net/`）は IPv4 だけ（ARP・ICMP・IPv4・inet socket）。`AF_INET6` は `include/uapi/socket.h` に定数だけある。
- IPv6 の本体・ICMPv6・NDP・SLAAC・`AF_INET6` の socket と、userland（`dhcpc -6`、`getaddrinfo` の IPv6、ping・ifconfig・route の IPv6、networkd の IPv6 の route と DNS）を設計し、ベータ2 以降に Phase に分けて実装する。

## 決まっていること

- DHCPv6 は既存の `dhcpc` に `-6` を足す（ユーザー）。

## 決まったこと（2026-10-03 ユーザー）

- 「SLAACは、カーネルはRAを受け取る。networkdが解決する、でいいと思います。networkdはインタフェースを開いて通知を監視できますしね。」→ NDP（近隣の cache・NS/NA・DAD・到達性・redirect）は kernel、RA は kernel が受けて networkd へ通知し、SLAAC の address・route・DNS（RDNSS）は networkd が決めて設定する。
- 「IPv6について、net.confの記法を ipv4: と ipv6: に分離する必要がありますね。個別に無効にする設定も。」→ `net.conf` の interface ごとの設定を `ipv4:` と `ipv6:` の節に分け、それぞれを個別に無効にできる記法にする（既存の IPv4 の記法からの移行と互換を p001 で設計）。
- address の方式（2026-10-03 user「RFC 7217, RFC 8981は両方使いましょう。記録しておいてください。」で決定）: 主の address は RFC 7217 の安定な address（MAC を出さず、同じ network では同じ値）、外への接続には RFC 8981 の一時的な address を既定で有効（macOS・Windows・最近の Linux と同じ）。どちらも `net.conf` の `ipv6:` で切り替えられる。

## 設計の論点（p001 で選択肢を出してユーザーと決める）

- **NDP**: 近隣の cache・NS/NA・DAD・到達性の確認・redirect を、ARP と同じく kernel に置く案（BSD・Linux と同じ）。
- **SLAAC**: (1) kernel が RA を受けて address を作る（Linux の既定）、(2) kernel は RA を受けて userland に渡し、networkd が address・route・DNS（RDNSS）の方針を決めて設定する（BSD の rtsold に近い、O3 の networkd に一貫）。privacy address（RFC 8981）、stable な address（RFC 7217）の要否。
- DHCPv6 と RA の M/O の flag の扱い、DNS は RDNSS（RA）と DHCPv6 のどちらを優先するか。
- dual stack での route と resolver の決め方（ws005-p019 の有線優先の仕組みとの整合）。
- POSIX の `<netinet/in.h>`・`getaddrinfo`・`inet_pton` の IPv6 の範囲（WS001 の台帳）。

## Phase（案）

| Phase | 目的 | Status | 依存 |
| --- | --- | --- | --- |
| p001 | 設計（上の論点の選択肢と推奨、kernel と userland の境界、試験の方法） | planning | ユーザーとの議論 |
| p002〜 | 実装（p001 の後に分ける。ベータ2 以降） | planning | p001 |
