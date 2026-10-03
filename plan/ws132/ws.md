<!-- awesome-plan project=zedbsd record=ws132 -->

# WS132: /dev/system の電源管理と PnP の通知、自動 mount、Files の eject

<!-- awesome-plan-current:start -->
Status: planning
Primary Milestone: MG006
Related Milestones: MG003、MG004
Objectives: O2, O3
Parent: [Master](../master.md)
Queue: none
Resume point: p001（設計）。ベータ1 に入れるかはユーザーの判断待ち。
<!-- awesome-plan-current:end -->

## 目標（2026-10-03 ユーザー）

- 「ejectは挿入の通知もほしいですが、まだそういう機能がカーネルにないですよね。udev的なものです。」
- 「/dev/systemに電源管理とPnP通知、両方入れるのがいいと思います。何を通知してほしいかは、subscriberが指定すればいいと思います。」

## 範囲（p001 で設計）

1. kernel: device の追加・削除（USB storage・network・display・input ほか）と電源の事象を、`/dev/system` を open した subscriber に `read()`・`poll()` で渡す。subscriber は受け取りたい事象の種類を指定する（filter）。形式（例: 1 行の key=value）、あふれた時の扱い、複数の読み手、権限（誰がどの事象を読めるか）。電源管理（WS052 の `/dev/system` での制御）と同じ node に載せる。
2. userland: 自動 mount の小さな daemon（devd に近い）が storage の事象を受けて mount の方針を決め、利用者の unmount の権限を扱う。desktop（libkeiland の抽象化、Files）へ通知する。
3. Files の eject（[ws127-p002](../ws127/phase002/phase.md) で止めた項目）と、挿入の通知での Locations の更新。
4. Linux・FreeBSD の Keiland では libkeiland の OS の module（udev・devd）で同じ抽象化を提供する。
