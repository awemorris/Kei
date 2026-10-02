<!-- awesome-plan project=zedbsd record=ws116-p001 -->

# ws116-p001: Qt6 upstream移植範囲のレビュー

Parent: [WS116](../ws.md)
Status: planning
Disposition: normal
Primary Milestone: MG002（WSから継承）
Queue / attempts: none
Purpose / goal: Qt6 upstream移植範囲のレビュー
Prerequisites: WS117 p002（Linux の Qt6 の行別採否）、WS115 p001（共通の移植契約）。2026-10-02 改訂
Investigation bound: Queue選定時に対象・時間・検証上限を決める。

## Procedure / affected components

GTK4で学んだ共通点とQt6特有のqtbase/qtwayland/QPA/依存/描画/portalを調べ、ユーザーと移植範囲・代表app・受け入れを決定する。

## Clearance / verification

p002の具体scope/受け入れとWS096向け学習項目が定義できる。

## Standards / evidence / resume

[Guardrail](../../guardrail.md)、[C全文](../../coding-style.md)、[scoped方針](../../standards/ws114-gtk-qt-learning.md)、[automation](../../standards/automation.md)を実装前に確認する。新コードは全文規約、外部tarballはprovenance・license・patch分離。具体コマンド/版/結果/commit/環境/成果物/skip/制限は未実施（計画のみ）。各PhaseはQueueの個別scope承認後に実行する。前提未達・判断待ちならattemptをunclearedとして証拠/再開条件を残す。

Event ws114-gtk-qt-port-plan-20261002: 2026-10-02 user指示から作成。planning/Queue none。GitHub publication pending。

2026-10-02 / ws116-beta1-plan-20261002（redesign）: user の順序の更新（Linux の Qt6 の調査を WS117 で先に行う）により、入力を WS115 の最終知見から WS117 p002 と WS115 p001 に改訂。範囲: Qt の版（Linux で実測する Debian13 の 6.8 系か inventory の 6.11.2）、module（qtbase の Core/Gui/Widgets と qtwayland を最小、Qt Quick/qtdeclarative は判断）、同梱依存（zlib・pcre2・freetype・harfbuzz・libpng・libjpeg）か WS115 の package を使うか、fontconfig/ICU/glib の無効化、代表 app、renderer（raster＋wl_shm を最初）、受け入れ。p004/p002/p005 の具体 scope を確定する。目安 3h ＋ user の判断。Status は planning のまま。
