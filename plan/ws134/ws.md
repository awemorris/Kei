<!-- awesome-plan project=zedbsd record=ws134 -->

# WS134: システムモニターのアプリ（Analytic Spatial UI）

<!-- awesome-plan-current:start -->
Status: planning
Primary Milestone: MG006
Related Milestones: —
Objectives: O2
Parent: [Master](../master.md)
Queue: q649（P2、p001）
Resume point: p001（設計: 情報の出どころの調査と画面・3D・動きの設計）
<!-- awesome-plan-current:end -->

## 目標（2026-10-03 ユーザー）

「P2が空いているので、システムモニターのアプリを作ってほしいです。添付がイメージです。」2026-10-03 user「イメージの通りじゃなくてよくて、要素を採用してほしいです。」→ 参考の画像の再現ではなく、コンセプトと画像の要素を取り入れて設計する。デザインのコンセプトは [design/user-concept.md](design/user-concept.md)（原文の要約）と参考の画像 [design/reference-image.webp](design/reference-image.webp)。

CPU（全体と各 core）、GPU（Util・VRAM・温度・電力、複数）、RAM（Used・Cache・Available・Swap）、Network（RX/TX）、Disk I/O（読み・書き・Latency）と最近の出来事を、中央の「システム状態の立体コア」を顔にした層構造の画面で見せる Keiland の app。3D は階層を見せるため、動きは「呼吸」。タブレットの操作（タップで浮く、スワイプで時間軸、長押しで固定、2 本指で俯瞰）。

## Phase

| Phase | 内容 | 状態 | 依存 |
| --- | --- | --- | --- |
| [p001](phase001/phase.md) | 設計: zedBSD（と Linux・FreeBSD）で取れる情報の出どころの調査（不足は kernel・libkeiland の追加の案）、画面の構成・3D の表現・動き・操作・描画の方式（libkeiui と Vulkan）、Phase の分け方 | planning（P2、q649） | — |

2026-10-03 user「P2はコードを書いてOKだと思います。衝突しないです。」 → P2 は設計（p001）を書いたら、user のレビューを待たずに実装の Phase へ進んでよい（Phase の ID は Q1 が割り当て）。

2026-10-03 user「P2はとりあえずOSから取れない情報はスタブデータでそれっぽいアニメーションを表示しましょう。」→ OS から今取れない値（GPU の温度・電力など）は、stub のデータ（もっともらしく動く値）で画面と動きを先に作る。stub であることは code と画面の上で分かるように（後で本物に差し替える所を一覧に）。
2026-10-03 user「simという表記はつけなくていいです。私がどの項目がスタブか把握できていればいいです。」→ 画面に stub の印は付けない。stub の項目の一覧を design.md（と完了の報告）に書き、user が把握できるようにする。
