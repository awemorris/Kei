<!-- awesome-plan project=zedbsd record=ws115-p002 -->

# ws115-p002: upstream GTK4 package移植・実行

Parent: [WS115](../ws.md)
Status: planning
Disposition: normal
Primary Milestone: MG002（WSから継承）
Queue / attempts: none
Purpose / goal: upstream GTK4 本体の package（`userland/packages/desktop/gtk4`）の build/install（2026-10-02 改訂: zedBSD 上の実行は p010 へ）
Prerequisites: p001契約、p007・p008（描画系）、p009（libwayland 互換）
Investigation bound: Queue選定時に対象・時間・検証上限を決める。

## Procedure / affected components

external.mkと公式tarball/patchでuserland/packages/desktop/gtk4を追加。zedBSD build/install/Wayland起動/操作を検証し、Linuxとの差を記録する。

## Clearance / verification

決めたapp/操作がtargetで通り、失敗とrenderer/fallbackを区別する。

## Standards / evidence / resume

[Guardrail](../../guardrail.md)、[C全文](../../coding-style.md)、[scoped方針](../../standards/ws114-gtk-qt-learning.md)、[automation](../../standards/automation.md)を実装前に確認する。新コードは全文規約、外部tarballはprovenance・license・patch分離。具体コマンド/版/結果/commit/環境/成果物/skip/制限は未実施（計画のみ）。各PhaseはQueueの個別scope承認後に実行する。前提未達・判断待ちならattemptをunclearedとして証拠/再開条件を残す。

Event ws114-gtk-qt-port-plan-20261002: 2026-10-02 user指示から作成。planning/Queue none。GitHub publication pending。

2026-10-02 / ws115-beta1-plan-20261002（redesign）: 依存 package を p004〜p009 に分け、本 Phase は GTK4 本体の `external.mk` 定義・公式 tarball・patch・meson 構成（p001 の任意機能の無効化）・build・sysroot/image への install・license audit・ELF の NEEDED/SONAME の確認に限る。zedBSD 上の起動と操作は新しい [p010](../phase010/phase.md) へ移した。受け入れ: target 向けの `libgtk-4.so` と demo app が warning を記録した上で build され、image に入り、NEEDED が全て image 内で解決する。目安 4h。所有 path `userland/packages/desktop/gtk4/`・`plan/ws115/`。Status は planning のまま。[WS summary](../ws.md)。
