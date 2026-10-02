<!-- awesome-plan project=zedbsd record=ws114-p002 -->

# ws114-p002: GTK4機能の採否レビュー

Parent: [WS114](../ws.md)
Status: planned（ユーザーの判断の席。Queue は不要、判断の記録は main/担当が行う）
Disposition: normal
Primary Milestone: MG006（WSから継承）
Queue / attempts: none
Purpose / goal: GTK4機能の採否レビュー
Prerequisites: p001の実測と機能表
Investigation bound: Queue選定時に対象・時間・検証上限を決める。

## Procedure / affected components

G01–G19のうちG05以外をユーザーと行別に採用・限定採用・保留へ分類し、WS受け入れ/p003/p004の具体scopeと順番を確定する。

## Clearance / verification

判断の出典と対象機能、合否条件、依存を文書化する。ユーザー未判断の項目は勝手に採用しない。

## Standards / evidence / resume

[Guardrail](../../guardrail.md)、[C全文](../../coding-style.md)、[scoped方針](../../standards/ws114-gtk-qt-learning.md)、[automation](../../standards/automation.md)を実装前に確認する。新コードは全文規約、外部tarballはprovenance・license・patch分離。具体コマンド/版/結果/commit/環境/成果物/skip/制限は未実施（計画のみ）。各PhaseはQueueの個別scope承認後に実行する。前提未達・判断待ちならattemptをunclearedとして証拠/再開条件を残す。

Event ws114-gtk-qt-port-plan-20261002: 2026-10-02 user指示から作成。planning/Queue none。GitHub publication pending。

2026-10-02 / ws114-csd-user-selection-20261002: G05はユーザー個別採用済みとしてp007へ分離。他行の採否を引き続き記録し、p007の実行を全体review待ちにしない。[追加Phase](../phase007/phase.md)、[WS summary](../ws.md)。本Phaseはplanningのまま、Queue未投入。

## 推奨案（2026-10-02 計画担当。提案であり採否ではない）

ベータ1（2026-10-17）の残り日数と、WS117（Qt6）・WS115（zedBSD 移植）が同じ compositor の行を使うことから、次を提案する。WS117 p002 の Qt6 行と同じ席で決めると、共通行（activation・text-input・decoration・primary selection）を一度に判断できる。

| 区分 | 行 | 理由 |
| --- | --- | --- |
| 採用（基礎、p003） | G01〜G04、G06〜G09 の観測済み不足: G04 の restore 時の buffer 増加（728→756→784）、G08 の grab 後同 client の wheel/hover 不反応（q587 の release 修正で解消したかを先に確認） | 通常の GTK4 window の品質。修正の範囲が小さい |
| 限定採用（p003） | G10: Kei IME（WS095）の日本語入力を標準 GTK4 の text-input-v3 で確認（Linux で IME program を起動できる場合） | IME をベータ1で上げる user 指示と重なる。GTK4/Qt6 の両方で使う |
| 限定採用（p003、判断） | G12 xdg-activation、G11 fractional scale | Qt6 も使う。WS117 の実測で要否を決める |
| 保留（ベータ1の後） | G13〜G16 portal（→ p004 取消または保留）、G17 AT-SPI、G18 GNOME 全体、G19 native menubar（F-045） | 通常の window の起動に必須でない。q580/q581 で通常 FileDialog は動作 |

決めたら、各行の「ユーザー判断」の欄と p003/p004 の scope・受け入れを更新し、p004 を取消すならその理由と WS 受け入れの改訂を残す。
