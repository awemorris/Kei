<!-- awesome-plan project=zedbsd record=ws114-p001 -->

# ws114-p001: 標準Linux GTK4 baselineと機能表の実測

Parent: [WS114](../ws.md)
Status: cleared
Disposition: normal
Primary Milestone: MG006（WSから継承）
Queue / attempts: q581 / q581-i01 / B1; past q580-i01 uncleared
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

## q580 / generation2 wrap outcome（2026-10-02）

Attempt q580-i01: **uncleared**。userのA/B分割・全subagent停止指示で通常wrap。標準GTK4/demoの部分実測と全19行の観測/skip理由を保存。window/popup/modal/FileDialog通常経路、既定software GL renderer、同一clientのUnicode clipboard、maximize/fullscreen/scale1は証拠あり。interactive move/有効なresize・別client clipboard等は未測定でwhole-clearを主張しない。G04復帰寸法増加とG05二重decorationsはbaseline限定未確定bug候補、current runtimeとの同一性/既存Bug照合はmainへ送付、ticket未判定。本体修正/portal backend追加/採否決定無し。

[結果・残件](q580-result.md)、[機能表](../gtk4-compat-matrix.md)、[再開](../tests/README.md)、[停止証拠](../evidence/q580/stop.log)。QEMU251216/SSH2249正常停止、absence確認。overlayは専用ignoredbuildに保全、credentials非commit。新Queue/採否前のsource実装開始無し。main統合/判定/共有Board更新待ち、GitHub publication保留。

## q581 / B1 remaining baseline attempt

2026-10-02: user「では、N=3で作業を開始してください。」、B main dispatch q581-i01。最大3時間。承認snapshot `635c60678bd8d56d172f5f43fc5069866ea0f1f9dc515a3799972b4737715144`。保存package/sourceを固定しmove/resize、cross-client clipboard、wheel/focus/cursor、tooltip/reposition、Cairo/Vulkan診断を優先する。GTK/compositor source変更・採否・portal実装は含めない。B1専用SSH2249/QMPを使用。全行の証拠/skipと正常停止を保存して判定する。

## q581 result / 2026-10-02

Attempt q581-i01 **cleared**。G01–G19の行別review材料として、残主要操作と具体skip理由、実版/commands/QMP PNG/Wayland・application・D-Bus証拠を保存し、専用guestを正常停止。p001の調査基準を満たす。全機能合格・修正・採否ではない。CSD move/有効resize、別client Unicode clipboard、wheel/focus/cursor、tooltip/reposition、software Cairo/Vulkanの表示/入力を実測。二重装飾/復帰寸法は未修正、grab後入力状態のbaseline限定観測をmainへ報告。p002採否と追加CSD source Phaseは別。

[結果・手順・制限](q581-result.md)、[19行表](../gtk4-compat-matrix.md)、[停止](../evidence/q581/stop.log)。GitHub publication/closeとcanonical投影はmain待ち。
