<!-- awesome-plan project=zedbsd record=ws116-p002 -->

# ws116-p002: 決定済みQt6 moduleの移植

Parent: [WS116](../ws.md)
Status: planning
Disposition: normal
Primary Milestone: MG002（WSから継承）
Queue / attempts: none
Purpose / goal: 決定済みQt6 module の qtbase の cross build/install（2026-10-02 改訂: qtwayland と実行は p005）
Prerequisites: p001のユーザー決定、p004（host 道具）、WS115 p008（libxkbcommon）。現状はQueue選定不可
Investigation bound: Queue選定時に対象・時間・検証上限を決める。

## Procedure / affected components

ユーザーが選んだmoduleだけを公式tarball＋patchで移植し、zedBSD build/実操作を検証する。詳細scopeはp001後に改訂する。

## Clearance / verification

p001で合意した代表app/操作/rendererがtargetで通る。

## Standards / evidence / resume

[Guardrail](../../guardrail.md)、[C全文](../../coding-style.md)、[scoped方針](../../standards/ws114-gtk-qt-learning.md)、[automation](../../standards/automation.md)を実装前に確認する。新コードは全文規約、外部tarballはprovenance・license・patch分離。具体コマンド/版/結果/commit/環境/成果物/skip/制限は未実施（計画のみ）。各PhaseはQueueの個別scope承認後に実行する。前提未達・判断待ちならattemptをunclearedとして証拠/再開条件を残す。

Event ws114-gtk-qt-port-plan-20261002: 2026-10-02 user指示から作成。planning/Queue none。GitHub publication pending。

2026-10-02 / ws116-beta1-plan-20261002（redesign）: 本 Phase を `userland/packages/desktop/qt6` の qtbase（p001 で決めた module）の公式 tarball・patch・CMake の cross 構成（`external.mk` の CMake toolchain file、host 道具は p004 の `QT_HOST_PATH`）・build・install・license audit・ELF 確認に限る。qtwayland と zedBSD 上の実行は [p005](../phase005/phase.md) へ。受け入れ: target 向けの Qt6Core/Gui/Widgets が build され image に入り、guest で QCoreApplication の小さな program が動く。目安 4h。Status は planning のまま。
