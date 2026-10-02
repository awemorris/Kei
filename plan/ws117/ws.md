<!-- awesome-plan project=zedbsd record=ws117 -->

# WS117: Linux の本物の Qt6 の動作を調査し、素の Qt6 アプリが動くように compositor を改良する

<!-- awesome-plan-current:start -->
Status: planning
Primary Milestone: MG006
Related Milestones: MG002（WS115/WS116 の移植の前提）
Objectives: O2
Parent: [Master](../master.md)
Queue: none
Resume point: WS114 の Linux の本物の GTK4 の CSD 完成（p007）の後に p001（調査の範囲と機能表）を定義する。
<!-- awesome-plan-current:end -->

## 目標（2026-10-02 ユーザー）

「本物のGTK4のzedBSDへの移植に進む前に、Linuxで本物のQt6の動作を調査しましょう。ここで、素のQt6アプリを動かせるように、コンポジタの改良を行います。そのあとで、素のGTK4と素のQt6を移植します。」

- Linux の Keiland（WS105）の上で、distro の標準 build の Qt6（Wayland QPA）の素のアプリを実測し、WS114 の GTK4 機能表と同じ形で動作/不足を表にする。
- 素の Qt6 アプリが動くために要る compositor（Keiland）の改良を行う。改良の範囲は機能表を見てユーザーと決める。
- 境界は [WS114–117 方針](../standards/ws114-gtk-qt-learning.md)、compositor の GPU/OS 境界は [Guardrail](../guardrail.md)。

## 完了の条件（p001 で具体化）

1. 素の Qt6 アプリの代表（p001 で選ぶ）について、Linux Keiland での実測の機能表（ソース実装と guest 実測を分ける）。
2. ユーザーが採用した行の compositor の改良を実装し、素の Qt6 アプリが Linux Keiland で起動・操作できる。zedBSD の compositor の振る舞いを壊さない。
3. 変えた source の全文規約・build・回帰。WS115（GTK4 移植）・WS116（Qt6 移植）へ知見を渡す。

## Phase

| Phase | 目的 | Status | 依存 |
| --- | --- | --- | --- |
| p001 | Qt6 の代表アプリと調査範囲、実測と機能表 | planning | WS114 p007 cleared |
| p002〜 | 採用した行の compositor の改良（p001 の後に分ける） | planning | p001、ユーザーの行別採否 |
| 最後 | 全文規約と回帰 | planning | 実装 Phase |
