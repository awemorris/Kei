<!-- awesome-plan project=zedbsd record=ws115-p007 -->

# ws115-p007: pixman・cairo・fribidi・pango

Parent: [WS115](../ws.md)
Status: planning
Disposition: normal
Primary Milestone: MG002（WSから継承）
Queue / attempts: none
Purpose / goal: GTK4 の text と Cairo renderer の基盤を移植する。
Prerequisites: p006
Investigation bound: timebox 4h
Origin: WS034 p028 の前半（移管を main に依頼）

## 範囲

- `userland/packages/libs/{pixman,cairo,fribidi,pango}`。cairo は image surface と PNG だけ（X11・GL は無効）。pango は fontconfig/freetype/harfbuzz の backend。
- pixman の SIMD の option は target の CPU に合わせる。

## 受け入れ

4 package が build・install、license audit。guest で cairo の image surface に pango で日本語を含む文字列を描いた PNG を作り、host で目視確認できる（PNG はユーザーに見せる）。

## 所有 path

`userland/packages/libs/{pixman,cairo,fribidi,pango}/`、`plan/ws115/`

## Standards / evidence / resume

[Guardrail](../../guardrail.md)、[C全文](../../coding-style.md)、[scoped方針](../../standards/ws114-gtk-qt-learning.md)、[automation](../../standards/automation.md)、[license audit](../../tools/packages/audit-licenses.sh) を実装前に確認する。外部 package は公式 tarball を取得・検証して patch し、source tree へ取り込まない（AGENTS.md）。新コードは全文規約。toolchain（`toolchain/`・`lang/clang`・`devel/libcxx`・共有 `build/llvm` 等）は変更・build しない。共有 `build/` は読取専用、自分の worktree の `build/` を使う。全体の `make check` は禁止。具体コマンド/版/結果/commit/環境/成果物/skip/制限は未実施（計画のみ）。各PhaseはQueueの個別scope承認後に実行する。前提未達・判断待ちならattemptをunclearedとして証拠/再開条件を残す。

Event ws115-beta1-plan-20261002: 2026-10-02 計画担当が依存 package の移植 Phase として作成。版・option は p001 の移植契約で確定する。planning/Queue none。GitHub publication pending。
