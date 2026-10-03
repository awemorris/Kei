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
| [p002](phase002/phase.md) | M1 骨組み: package・3 OS の build・libkeiui の窓＋自前の Vulkan・title bar・data source の層（sim・replay）・履歴・plate と文字・ZMON の log・App Home | in-progress（P2: 実装済み・T1 の試験待ち） | p001 |
| p003 | M2 3D と動き: 状態コア・CPU のタイル面・GPU のカード・Network/Disk の流れ・Memory の層・視差・数値の slide・段階的な警告・Events | planned | p002 |
| p004 | M3a 操作: tap の展開・長押しの固定・swipe の時間軸・pinch の俯瞰・key/pointer・--calm | planned | p003 |
| p005 | K1 kernel: CPU ごとの時間 `hw.cputimes`（sysctl の CLI・top の CPU 行、design.md §1.2 の review の反映） | planned | — |
| p006 | K2 kernel: disk ごとの統計 `hw.diskstats`（物理の whole disk） | planned | — |
| p007 | K3 kernel: GPU の telemetry の sysctl `hw.gputelemetry`（Guardrail により ioctl にしない。実機の確認は 5330） | planned | — |
| p008 | M3a 本物の値（zedBSD の backend の monitor の領域） | planned | WS131 p010 までの統合、p005・p006 |
| p011 | M3b Linux・FreeBSD の backend の monitor の領域 | planned | p008 |
| p012 | M3c compositor の `kl_system_monitor_v1`（manager v3、専用の thread、ack と間引き）と libkeiland の `kl_system_monitor_*` | planned | p008 |
| p013 | M3d app の system の source | planned | p012 |
| p009 | K4 kernel: ACPI の thermal・電池（実機、WS131 p005 と調整） | planned | 実機 |
| p010 | M4 全文規約・回帰・デモの通し | planned | p002〜p009 |

2026-10-03 user「P2はコードを書いてOKだと思います。衝突しないです。」 → P2 は設計（p001）を書いたら、user のレビューを待たずに実装の Phase へ進んでよい（Phase の ID は Q1 が割り当て）。

2026-10-03 user「P2はとりあえずOSから取れない情報はスタブデータでそれっぽいアニメーションを表示しましょう。」→ OS から今取れない値（GPU の温度・電力など）は、stub のデータ（もっともらしく動く値）で画面と動きを先に作る。stub であることは code と画面の上で分かるように（後で本物に差し替える所を一覧に）。
2026-10-03 user「simという表記はつけなくていいです。私がどの項目がスタブか把握できていればいいです。」→ 画面に stub の印は付けない。stub の項目の一覧を design.md（と完了の報告）に書き、user が把握できるようにする。

2026-10-03 user「システムモニターは私に確認しなくていいので、どんどん実装して動かしてください。」 → P2 は WS134 の Phase を user の確認なしに順に進める（Q1 は報告だけ受ける）。
