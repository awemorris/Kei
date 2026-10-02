<!-- awesome-plan project=zedbsd record=ws122 -->

# WS122: 動画プレーヤアプリ

<!-- awesome-plan-current:start -->
Status: planning
Primary Milestone: MG006
Objectives: O2
Parent: [Master](../master.md)
Focused goal: fg019（ベータ1）
Queue: none
Resume point: p001（要件・設計）。
2026-10-02 user: 動画関連（WS083・WS121・WS122）は**別セッション**でユーザーがノウハウを提供しステップバイステップで進める。このセッション（Q1/P1〜P8）は割り当てない。ベータ1 では最悪 drop してもリリース可能とする（努力目標）。VA-API（WS123）は canceled、アプリが Vulkan Video を直接使う。ブラウザへの組み込み（WS121）は Codex 側と調整。
<!-- awesome-plan-current:end -->

## 目標（2026-10-02 ユーザー（ベータ1、リリース目標 10/17））

「動画プレーヤアプリを追加する。」

- Keiland の動画プレーヤ。Vulkan Video（WS083）を直接使いで H.264 を decode し、表示・音声・seek を行う。

## Phase

| Phase | 目的 | Status | 依存 |
| --- | --- | --- | --- |
| p001 | 要件・設計 | planning | — |
| 最後 | 全文規約と回帰 | planning | 実装 Phase |
