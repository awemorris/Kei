<!-- awesome-plan project=zedbsd record=ws096 -->

# WS096: Qt6（core・gui・widgets）の互換の実装（完全な書き下ろし、zlib）

<!-- awesome-plan-current:start -->
Status: planning
Primary Milestone: MG002
Related Milestones: MG006
Objectives: O2
Parent: [Master](../master.md)
Queue: なし
Resume point: p001（設計）から
<!-- awesome-plan-current:end -->

## 目標（2026-09-29 ユーザー）

「デモ当日以降でよいが、Qt6のcore, gui, widgetsと、GTK4について、完全書き下ろしの互換実装を行う。APIのインタフェースだけ利用させてもらう。ライセンスはzlib。」

- Qt6 の QtCore・QtGui・QtWidgets の公開の API（header の interface）に合わせた、完全な書き下ろしの実装。Qt の source は使わない（API の
  interface だけ）。license は zlib。Kei の Wayland・Vulkan・WS090 の部品の上。**デモの後**。

## Phase（案）

| Phase | 目的 | Status | 依存 |
| --- | --- | --- | --- |
| ws096-p001 | 設計 | planning | — |

## 2026-10-02 / upstream学習との関係

Event ws114-gtk-qt-port-projections-20261002: ユーザー回答「書き下ろし計画も残す」および追加指示により、完全書き下ろし/zlib/API interfaceのみ/デモ後の方針を保持。先に[WS114](../ws114/ws.md)のGTK4互換性、[WS115](../ws115/ws.md)のGTK4移植、[WS116](../ws116/ws.md)のQt6移植で要るOS API・QPA・Wayland/描画/portalの知見を得てからp001を再設計する。WS116の実装範囲はGTK4学習後のユーザー判断。Qt upstream sourceをこのWSへ転用しない。Queue none、実装なし。
