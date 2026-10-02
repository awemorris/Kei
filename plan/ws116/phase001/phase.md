<!-- awesome-plan project=zedbsd record=ws116-p001 -->

# ws116-p001: Qt6 upstream移植範囲のレビュー

Parent: [WS116](../ws.md)
Status: planning
Disposition: normal
Primary Milestone: MG002（WSから継承）
Queue / attempts: none
Purpose / goal: Qt6 upstream移植範囲のレビュー
Prerequisites: WS115の最終移植知見
Investigation bound: Queue選定時に対象・時間・検証上限を決める。

## Procedure / affected components

GTK4で学んだ共通点とQt6特有のqtbase/qtwayland/QPA/依存/描画/portalを調べ、ユーザーと移植範囲・代表app・受け入れを決定する。

## Clearance / verification

p002の具体scope/受け入れとWS096向け学習項目が定義できる。

## Standards / evidence / resume

[Guardrail](../../guardrail.md)、[C全文](../../coding-style.md)、[scoped方針](../../standards/ws114-gtk-qt-learning.md)、[automation](../../standards/automation.md)を実装前に確認する。新コードは全文規約、外部tarballはprovenance・license・patch分離。具体コマンド/版/結果/commit/環境/成果物/skip/制限は未実施（計画のみ）。各PhaseはQueueの個別scope承認後に実行する。前提未達・判断待ちならattemptをunclearedとして証拠/再開条件を残す。

Event ws114-gtk-qt-port-plan-20261002: 2026-10-02 user指示から作成。planning/Queue none。GitHub publication pending。
