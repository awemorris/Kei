<!-- awesome-plan project=zedbsd record=ws115-p006 -->

# ws115-p006: libpng・freetype・harfbuzz・fontconfig

Parent: [WS115](../ws.md)
Status: in-progress（q605-i01、P3）
Disposition: normal
Primary Milestone: MG002（WSから継承）
Queue / attempts: q605-i01（P3、継続 dispatch、時限 4h、base main 55ff880b4）
Purpose / goal: 文字の描画の基盤の library を移植する。
Prerequisites: p005（harfbuzz は glib/gobject 付き）
Investigation bound: timebox 4h
Origin: WS034 p027（移管を main に依頼）

## 範囲

- `userland/packages/libs/{libpng,freetype,harfbuzz,fontconfig}`。freetype を harfbuzz 無しで作り harfbuzz を作る（循環の扱いは inventory §2.5）。fontconfig は host の gperf が要る（p004）。harfbuzz は C++（libc++）。
- fontconfig の設定（`/usr/share/fonts` と zedBSD の既存の font の場所、cache の生成）。
- 共通の依存として WS116（Qt6）でも使う前提で option を決める。

## 受け入れ

4 package が build・install、license audit。guest で `fc-list` が zedBSD の font を列挙し、harfbuzz の shape の小さな試験が通る。

## 所有 path

`userland/packages/libs/{libpng,freetype,harfbuzz,fontconfig}/`、`plan/ws115/`

## Standards / evidence / resume

[Guardrail](../../guardrail.md)、[C全文](../../coding-style.md)、[scoped方針](../../standards/ws114-gtk-qt-learning.md)、[automation](../../standards/automation.md)、[license audit](../../tools/packages/audit-licenses.sh) を実装前に確認する。外部 package は公式 tarball を取得・検証して patch し、source tree へ取り込まない（AGENTS.md）。新コードは全文規約。toolchain（`toolchain/`・`lang/clang`・`devel/libcxx`・共有 `build/llvm` 等）は変更・build しない。共有 `build/` は読取専用、自分の worktree の `build/` を使う。全体の `make check` は禁止。具体コマンド/版/結果/commit/環境/成果物/skip/制限は未実施（計画のみ）。各PhaseはQueueの個別scope承認後に実行する。前提未達・判断待ちならattemptをunclearedとして証拠/再開条件を残す。

Event ws115-beta1-plan-20261002: 2026-10-02 計画担当が依存 package の移植 Phase として作成。版・option は p001 の移植契約で確定する。planning/Queue none。GitHub publication pending。
