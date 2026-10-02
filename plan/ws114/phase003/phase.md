<!-- awesome-plan project=zedbsd record=ws114-p003 -->

# ws114-p003: 選択されたXDG-shell/compositorの改善

Parent: [WS114](../ws.md)
Status: planning
Disposition: normal
Primary Milestone: MG006（WSから継承）
Queue / attempts: none
Purpose / goal: 選択されたXDG-shell/compositorの改善
Prerequisites: p002の採用済み機能と再現例
Investigation bound: Queue選定時に対象・時間・検証上限を決める。

## Procedure / affected components

G01–G12のうちG05以外の採用された問題についてLinux compositorのprotocol/描画/入力を修正し、標準GTK4で再現/回帰する。具体差分と手順はp002で限定する。

## Clearance / verification

採用された各経路の操作とrenderer結果をguestで確認する。未採用機能を含めない。

## Standards / evidence / resume

[Guardrail](../../guardrail.md)、[C全文](../../coding-style.md)、[scoped方針](../../standards/ws114-gtk-qt-learning.md)、[automation](../../standards/automation.md)を実装前に確認する。新コードは全文規約、外部tarballはprovenance・license・patch分離。具体コマンド/版/結果/commit/環境/成果物/skip/制限は未実施（計画のみ）。各PhaseはQueueの個別scope承認後に実行する。前提未達・判断待ちならattemptをunclearedとして証拠/再開条件を残す。

Event ws114-gtk-qt-port-plan-20261002: 2026-10-02 user指示から作成。planning/Queue none。GitHub publication pending。

2026-10-02 / ws114-csd-user-selection-20261002: G05の装飾モード改善はp007に分離し重複実装しない。他の改善はp002の具体採用判断を待つ。[追加Phase](../phase007/phase.md)、[WS summary](../ws.md)。本Phaseはplanningのまま、Queue未投入。
