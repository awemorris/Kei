<!-- awesome-plan project=zedbsd record=ws115 -->

# WS115: upstream GTK4 を zedBSD の desktop package に移植する

Status: planning
Primary Milestone: MG002
Related Milestones: MG006（GUI動作、compositor互換性）
Parent: [Master](../master.md)
Queue: none / 計画のみ
Resume point: WS114で承認した機能とLinux実測を入力にp001の移植契約を作る。

## Objective / scope

外部のGTK4ソースを公式tarball＋zedBSD patchとして `userland/packages/desktop/gtk4/` に追加し、zedBSD上でWaylandのGTK4アプリをbuild/実行する。移植時の実際の依存・OS API・描画・入力・Wayland/portal条件を記録し、後の完全な書き下ろし [WS097](../ws097/ws.md) の設計材料にする。GTK4ソースをbase/compositorへ取り込まない。既存[WS034のinventory](../ws034/package-inventory.md)を再検証して使う。

## Completion criteria（p001で対象app/操作を確定）

1. tarball版・出典・hash・license・patchを記録し、zedBSD target向けGTK4と必要依存がbuild/installできる。
2. zedBSD amd64 QEMU/Venusで選択したGTK4アプリがWayland上でwindowを表示し、p001で定めた代表操作を行える。Vulkan/ソフトウェアのどのrendererで成功したか分ける。
3. 必要だったOS API/第三者ライブラリ/Wayland protocol/portalと各patchの理由を引継ぎ表に残す。Linux標準GTK4との差分を示す。
4. 全変更sourceとpackage metadataの全文規約・provenance・build/guest回帰を検証する。

## Dependencies / ownership

[WS114](../ws114/ws.md)の採用範囲/実測が前提。[WS034](../ws034/ws.md) p028（描画系）/p034（本家libwayland）とp038（zedBSDのEGL/Vulkan横断調査）の必要な成果を確認。p029の旧GTK4実装枠はこのWSへ移す。p028/p034/p038の他package目的は維持する。Linuxでの標準GTK4の成功はzedBSD動作の証拠にしない。native menubarの後日案[F-045](../future-work.md)は通常GTK4移植の必須条件にせず、WS114の機能表G19でレビューする。[Guardrail](../guardrail.md)、[C全文](../coding-style.md)、[方針](../standards/ws114-gtk-qt-learning.md)、[license audit](../tools/packages/audit-licenses.sh)。既存のinventory版は候補であり、実装時に公式入力と依存を再照合する。

## Phases

| ID | Purpose / goal | Status | Dependencies |
| --- | --- | --- | --- |
| [ws115-p001](phase001/phase.md) | target app/依存/patch/renderer/試験契約 | planning | WS114の実測と採用範囲、WS034必要成果 |
| [ws115-p002](phase002/phase.md) | upstream GTK4の移植・build/zedBSD実行 | planning | p001契約/必要依存 |
| [ws115-p003](phase003/phase.md) | 知見引継ぎ・最終全文規約と回帰 | planning | p002の最終sourceとguest証拠 |

Graph: WS114 + WS034 context/必要出力 → p001 → p002 → p003 → WS097/WS116。Queueなし。具体patchや新OS APIはp001調査後に範囲/判断を確定。

## Event

2026-10-02 / ws114-gtk-qt-port-plan-20261002: upstream GTK4移植をWS034 p029から別WSへ移管計画。独自実装WS097は保持。実装/guest試験なし、GitHub publication pending。
