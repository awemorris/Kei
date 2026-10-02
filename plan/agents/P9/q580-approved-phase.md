<!-- awesome-plan project=zedbsd record=ws114-p001 -->

# ws114-p001: 標準Linux GTK4 baselineと機能表の実測

Parent: [WS114](../ws.md)
Status: in-progress
Disposition: normal
Primary Milestone: MG006（WSから継承）
Queue / attempts: q580 / q580-i01 / P9 generation2
Purpose / goal: 標準Linux GTK4 baselineと機能表の実測
Prerequisites: WS105のLinux版guest/compositorが使えること（既存成果の再確認）
Investigation bound: 最大3時間。Debian13の既存専用guestをclone/overlayで使用、標準GTK4/gtk4-demoでG01–G11の基本経路、G12–G19は観測/未試験理由を記録。compositor/GTK本体の変更無し。採否はp002でuser判断。

## Procedure / affected components

Debian 13等の標準GTK4版・実アプリ・Wayland globals・rendererを固定し、window/menu/dialog/input/clipboardとportal呼出しを有限に実測。結果で機能表G01–G18の「未検証」を更新する。compositor/GTKソース変更は含めない。

## Clearance / verification

各行に実測または試験しない理由、再現手順、stdout/stderr、QMP画面、環境を記録し、ユーザーが採否を決める材料になる。

## Standards / evidence / resume

[Guardrail](../../guardrail.md)、[C全文](../../coding-style.md)、[scoped方針](../../standards/ws114-gtk-qt-learning.md)、[automation](../../standards/automation.md)を実装前に確認する。新コードは全文規約、外部tarballはprovenance・license・patch分離。具体コマンド/版/結果/commit/環境/成果物/skip/制限は未実施（計画のみ）。各PhaseはQueueの個別scope承認後に実行する。前提未達・判断待ちならattemptをunclearedとして証拠/再開条件を残す。

Event ws114-gtk-qt-port-plan-20261002: 2026-10-02 user指示から作成。planning/Queue none。GitHub publication pending。

2026-10-02 / ws114-next-desktop-q580: user「デスクトップ担当には、次の作業として、GTK4の移植作業のWSを進めてもらいたいです。」および「利用上限は回復したので、サブエージェントを起動し直してOKです」。q578完了後の次scopeを既存p001 baseline実測に限定しq580で実行。Linux専用guest標準GTK4package導入/実操作/機能表証拠化、3時間。p002採否前にcompositor/portal実装を始めない。GitHub publication保留。
