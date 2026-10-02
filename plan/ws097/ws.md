<!-- awesome-plan project=zedbsd record=ws097 -->

# WS097: GTK4 の互換の実装（完全な書き下ろし、zlib）

<!-- awesome-plan-current:start -->
Status: planning
Primary Milestone: MG002
Related Milestones: MG006
Objectives: O2
Parent: [Master](../master.md)
Queue: なし
Resume point: p001（設計）から
2026-10-02（計画担当）: 前提の知見は WS114（Linux GTK4）と WS115（upstream GTK4 の zedBSD 移植）から受ける。ベータ1（fg019）の対象外。
<!-- awesome-plan-current:end -->

## 目標（2026-09-29 ユーザー）

「デモ当日以降でよいが、Qt6のcore, gui, widgetsと、GTK4について、完全書き下ろしの互換実装を行う。APIのインタフェースだけ利用させてもらう。ライセンスはzlib。」

- GTK4（と要る範囲の GLib・GObject・GDK）の公開の API に合わせた、完全な書き下ろしの実装。GTK の source は使わない（API の interface だけ）。
  license は zlib。**デモの後**。

## Phase（案）

| Phase | 目的 | Status | 依存 |
| --- | --- | --- | --- |
| ws097-p001 | 設計 | planning | — |

## 2026-10-02 / upstream学習との関係

Event ws114-gtk-qt-port-projections-20261002: ユーザー回答「書き下ろし計画も残す」および追加指示により、完全書き下ろし/zlib/API interfaceのみ/デモ後の方針を保持。[WS114](../ws114/ws.md)のLinux標準GTK4互換性実測と[WS115](../ws115/ws.md)のzedBSD upstream移植で要るOS API・ライブラリ・Wayland/描画/portalの知見を得てからp001を再設計する。GTK upstream sourceをこのWSへ転用しない。Queue none、実装なし。
