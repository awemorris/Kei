<!-- awesome-plan project=zedbsd record=ws115-p008 -->

# ws115-p008: gdk-pixbuf・libjpeg-turbo・libtiff・graphene・libepoxy・libxkbcommon・xkeyboard-config

Parent: [WS115](../ws.md)
Status: in-progress（q610-i01、P3）
Disposition: normal
Primary Milestone: MG002（WSから継承）
Queue / attempts: q610-i01（P3、継続 dispatch、時限 4h、base main 9f408ea0c）
Purpose / goal: GTK4 の残りの必須依存を移植する。
Prerequisites: p005（p007 と並行可）
Investigation bound: timebox 4h。収まらなければ libxkbcommon/xkeyboard-config を別 Queue に分ける
Origin: WS034 p028 の後半（移管を main に依頼）

## 範囲

- `userland/packages/libs/{gdk-pixbuf,libjpeg-turbo,libtiff,graphene,libepoxy,libxkbcommon}` と xkeyboard-config の data。gdk-pixbuf の loader は builtin、introspection 無し。libepoxy は EGL/GLES を zedBSD の libegl/libglesv2 で解決するか、GL 無しで build するか（p001 の方針）。libxkbcommon は Wayland の keymap の解析に要る（compositor が送る keymap と合うかを確認）。

## 受け入れ

全 package が build・install、license audit。guest で gdk-pixbuf の PNG/JPEG 読み込み、libxkbcommon で compositor の keymap の解析の小さな試験。

## 所有 path

`userland/packages/libs/` の上記、`plan/ws115/`

## Standards / evidence / resume

[Guardrail](../../guardrail.md)、[C全文](../../coding-style.md)、[scoped方針](../../standards/ws114-gtk-qt-learning.md)、[automation](../../standards/automation.md)、[license audit](../../tools/packages/audit-licenses.sh) を実装前に確認する。外部 package は公式 tarball を取得・検証して patch し、source tree へ取り込まない（AGENTS.md）。新コードは全文規約。toolchain（`toolchain/`・`lang/clang`・`devel/libcxx`・共有 `build/llvm` 等）は変更・build しない。共有 `build/` は読取専用、自分の worktree の `build/` を使う。全体の `make check` は禁止。具体コマンド/版/結果/commit/環境/成果物/skip/制限は未実施（計画のみ）。各PhaseはQueueの個別scope承認後に実行する。前提未達・判断待ちならattemptをunclearedとして証拠/再開条件を残す。

Event ws115-beta1-plan-20261002: 2026-10-02 計画担当が依存 package の移植 Phase として作成。版・option は p001 の移植契約で確定する。planning/Queue none。GitHub publication pending。
