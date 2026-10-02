<!-- awesome-plan project=zedbsd record=ws114-p004 -->

# ws114-p004: 選択されたportal/session統合

Parent: [WS114](../ws.md)
Status: planning
Disposition: normal
Primary Milestone: MG006（WSから継承）
Queue / attempts: none
Purpose / goal: 選択されたportal/session統合
Prerequisites: p002で採用されたportal機能。選択ゼロなら取消/依存改訂
Investigation bound: Queue選定時に対象・時間・検証上限を決める。

## Procedure / affected components

G13–G16のうち採用されたものについてsession D-Bus/frontend/backend選択・既存backend再利用または必要な実装を設計し、実アプリから確認する。compositor必須機能とは決めつけない。

## Clearance / verification

採用portalの呼出しがGTK4実アプリで通る。選択しない場合はPhaseを取消して理由を残す。

## Standards / evidence / resume

[Guardrail](../../guardrail.md)、[C全文](../../coding-style.md)、[scoped方針](../../standards/ws114-gtk-qt-learning.md)、[automation](../../standards/automation.md)を実装前に確認する。新コードは全文規約、外部tarballはprovenance・license・patch分離。具体コマンド/版/結果/commit/環境/成果物/skip/制限は未実施（計画のみ）。各PhaseはQueueの個別scope承認後に実行する。前提未達・判断待ちならattemptをunclearedとして証拠/再開条件を残す。

Event ws114-gtk-qt-port-plan-20261002: 2026-10-02 user指示から作成。planning/Queue none。GitHub publication pending。

2026-10-02 / ws114-beta1-plan-20261002: 推奨はベータ1では portal を採用せず、本 Phase を取消（canceled、理由: 通常 FileDialog 経路で足り、portal backend は fg019 の範囲外）または保留とすること。ユーザーの p002 の判断で決める。Status は planning のまま。
