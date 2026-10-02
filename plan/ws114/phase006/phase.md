<!-- awesome-plan project=zedbsd record=ws114-p006 -->

# ws114-p006: 最終全文規約・回帰

Parent: [WS114](../ws.md)
Status: planning
Disposition: normal
Primary Milestone: MG006（WSから継承）
Queue / attempts: none
Purpose / goal: 最終全文規約・回帰
Prerequisites: p005の証拠と最終source
Investigation bound: Queue選定時に対象・時間・検証上限を決める。

## Procedure / affected components

本WSが変更した全sourceのC全文規約、formatter/style-check、Linux build、GTK4 guest操作を最終版で確認する。

## Clearance / verification

必要な規約/target回帰を通し、版/結果/例外/skip/制限を記録する。

## Standards / evidence / resume

[Guardrail](../../guardrail.md)、[C全文](../../coding-style.md)、[scoped方針](../../standards/ws114-gtk-qt-learning.md)、[automation](../../standards/automation.md)を実装前に確認する。新コードは全文規約、外部tarballはprovenance・license・patch分離。具体コマンド/版/結果/commit/環境/成果物/skip/制限は未実施（計画のみ）。各PhaseはQueueの個別scope承認後に実行する。前提未達・判断待ちならattemptをunclearedとして証拠/再開条件を残す。

Event ws114-gtk-qt-port-plan-20261002: 2026-10-02 user指示から作成。planning/Queue none。GitHub publication pending。
