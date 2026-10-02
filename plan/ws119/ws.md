<!-- awesome-plan project=zedbsd record=ws119 -->

# WS119: インストーラの作り直し

<!-- awesome-plan-current:start -->
Status: planning
Primary Milestone: MG003
Related Milestones: MG006
Objectives: O2
Parent: [Master](../master.md)
Queue: none
Resume point: 要件（対象・画面・partition・既存の WS019 の扱い）をユーザーと決める。
<!-- awesome-plan-current:end -->

## 目標（2026-10-02 ユーザー）

「インストーラは作り直しです。Linuxパッケージよりも上の優先度です。」

- ベータ1 に向けてインストーラを作り直す。**Wayland で動くインストーラ**（2026-10-02 user「Waylandで動くインストーラを実装する。」）。順位は WS118（5320）の後。旧インストーラ（[WS019](../ws019/ws.md)、completed）と 4 機種の実機の受け入れ（[WS028](../ws028/ws.md)）は前の世代の証拠として参照し、再利用しない（完了した WS に目標を足さない規則）。
- 要件（GUI か TUI か、partition の方式、対象の機種・媒体、既存の disk の扱い）はユーザーと決める。決まるまで Phase は planning。

## Phase

| Phase | 目的 | Status | 依存 |
| --- | --- | --- | --- |
| p001 | 要件と旧インストーラの調査、設計 | planning | ユーザーとの要件の議論 |
