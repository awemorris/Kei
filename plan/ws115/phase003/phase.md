<!-- awesome-plan project=zedbsd record=ws115-p003 -->

# ws115-p003: 移植知見と最終全文規約

Parent: [WS115](../ws.md)
Status: planning
Disposition: normal
Primary Milestone: MG002（WSから継承）
Queue / attempts: none
Purpose / goal: 移植知見と最終全文規約
Prerequisites: p010 の最終 source/target 証拠（2026-10-02 改訂）
Investigation bound: Queue選定時に対象・時間・検証上限を決める。

## Procedure / affected components

全WS変更の全文規約、formatter/style-check、package provenance/license/ELF、build/guest回帰を照合し、WS097/WS116向け知見を整理する。

## Clearance / verification

出典/パッチ/試験/制限の正確な引継ぎと最終ソース適合。

## Standards / evidence / resume

[Guardrail](../../guardrail.md)、[C全文](../../coding-style.md)、[scoped方針](../../standards/ws114-gtk-qt-learning.md)、[automation](../../standards/automation.md)を実装前に確認する。新コードは全文規約、外部tarballはprovenance・license・patch分離。具体コマンド/版/結果/commit/環境/成果物/skip/制限は未実施（計画のみ）。各PhaseはQueueの個別scope承認後に実行する。前提未達・判断待ちならattemptをunclearedとして証拠/再開条件を残す。

Event ws114-gtk-qt-port-plan-20261002: 2026-10-02 user指示から作成。planning/Queue none。GitHub publication pending。

2026-10-02 / ws115-beta1-plan-20261002（redesign）: 依存を p002 から p010 に改訂し、範囲に p004〜p009 の全 package（provenance・license・patch の理由）を含める。WS097 に加えて WS116（Qt6 の移植）へ共通の依存（freetype・harfbuzz・libpng・libxkbcommon・meson/CMake の契約）を渡す。目安 3h。Status は planning のまま。
