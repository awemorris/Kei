<!-- awesome-plan project=zedbsd record=ws115-p001 -->

# ws115-p001: GTK4 target移植契約

Parent: [WS115](../ws.md)
Status: planning
Disposition: normal
Primary Milestone: MG002（WSから継承）
Queue / attempts: none
Purpose / goal: GTK4 target移植契約
Prerequisites: WS114受け入れ/WS034必要成果
Investigation bound: Queue選定時に対象・時間・検証上限を決める。

## Procedure / affected components

GTK4 tarball版/hash/依存/OS API/patch場所、zedBSD上の代表app/renderer/操作/試験を選定し、ユーザーの採用済み機能に合わせる。

## Clearance / verification

実装するtarget・依存・有限の試験・license境界が明確である。

## Standards / evidence / resume

[Guardrail](../../guardrail.md)、[C全文](../../coding-style.md)、[scoped方針](../../standards/ws114-gtk-qt-learning.md)、[automation](../../standards/automation.md)を実装前に確認する。新コードは全文規約、外部tarballはprovenance・license・patch分離。具体コマンド/版/結果/commit/環境/成果物/skip/制限は未実施（計画のみ）。各PhaseはQueueの個別scope承認後に実行する。前提未達・判断待ちならattemptをunclearedとして証拠/再開条件を残す。

Event ws114-gtk-qt-port-plan-20261002: 2026-10-02 user指示から作成。planning/Queue none。GitHub publication pending。
