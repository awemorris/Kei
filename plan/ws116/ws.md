<!-- awesome-plan project=zedbsd record=ws116 -->

# WS116: upstream Qt6 の移植範囲を検討し、zedBSD packageへ移植する

Status: planning
Primary Milestone: MG002
Related Milestones: MG006（GUI動作、compositor互換性）
Parent: [Master](../master.md)
Queue: none / 計画のみ
Resume point: WS117（Linux 本物 Qt6 の調査と compositor 改良）と WS115 の後、p001 で Qt6 の実装範囲をユーザーと検討する（2026-10-02 user の順序）。

## Objective / scope

GTK4の先行移植を学んでから、Qt6 upstream のどのmodule・依存・Wayland QPA・機能をzedBSDへ移植するか決め、`userland/packages/desktop/qt6/` に公式tarball＋zedBSD patchを置いてbuild/実行する。結果を後のQt6完全書き下ろし [WS096](../ws096/ws.md)へ渡す。既存[WS034のinventory](../ws034/package-inventory.md)のqtbase/qtwaylandは候補であり、確定したscopeではない。

## Completion criteria（p001で具体化）

1. GTK4移植で判明した課題とQt6固有の要求を比較し、userがmodule/代表app/描画/Wayland/portal/受け入れ範囲を決める。
2. 決定したQt6 upstream packageの出典/hash/license/patch/依存を記録し、zedBSD targetでbuild/install、QEMU/Venusで代表appを起動/操作する。
3. Linux/GTK4と共通の対応、Qt6固有のOS API/QPA/Wayland/renderer条件を独自実装WS096へ引き渡す。全変更の全文規約、provenance、target build/実行を最終検証する。

## Dependencies / standards

[WS115](../ws115/ws.md)の移植・知見引継ぎが前提。旧[WS034](../ws034/ws.md) p030は移管先をここにする。WS034 p027/p028/p034等の依存はp001で必要性を照合。具体実装Phaseはp001で範囲確定後に追加/改訂する。native menubarの後日案[F-045](../future-work.md)はp001で必要性をレビューする。userが検討する前にQt6 codeを実装しない。[Guardrail](../guardrail.md)、[C全文](../coding-style.md)、[方針](../standards/ws114-gtk-qt-learning.md)。

## Phases

| ID | Purpose / goal | Status | Dependencies |
| --- | --- | --- | --- |
| [ws116-p001](phase001/phase.md) | Qt6 upstreamの必要範囲と実装方針のレビュー | planning | WS115の移植知見 |
| [ws116-p002](phase002/phase.md) | 決定したmoduleの移植/zedBSD実行（範囲は未決） | planning | p001のuser決定/依存 |
| [ws116-p003](phase003/phase.md) | 独自実装へ引継ぎ・最終全文規約/回帰 | planning | p002の最終source/guest証拠 |

Graph: WS115 → p001 → p002 → p003 → WS096。現時点でp002のscope/criteriaは提案前、Queue選定不可。

## Event

2026-10-02 / ws114-gtk-qt-port-plan-20261002: GTK4先行移植後にQt6範囲を検討するuser方針を記録。WS096書き下ろしは保持。実装/試験なし、GitHub publication pending。
