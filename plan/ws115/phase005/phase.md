<!-- awesome-plan project=zedbsd record=ws115-p005 -->

# ws115-p005: libffi・pcre2・glib

Parent: [WS115](../ws.md)
Status: planning
Disposition: normal
Primary Milestone: MG002（WSから継承）
Queue / attempts: none
Purpose / goal: GLib（gobject・gio・gmodule）を zedBSD target へ移植する。
Prerequisites: p004（meson の契約）、libc の不足の判断（p001）
Investigation bound: timebox 4h。libc の大きな不足が出たら uncleared で一覧を返す
Origin: WS034 p026（移管を main に依頼）

## 範囲

- `userland/packages/libs/{libffi,pcre2,glib}`。libffi は autotools＋libtool（version script を無効に）。glib の subproject の wrap はネットワークから取らせない（`--wrap-mode=nodownload`）。
- glib の file monitor・network・iconv・libintl の扱いは p001 の方針で patch か option。
- 同じ版の native の glib 道具（glib-compile-resources・glib-compile-schemas・glib-mkenums・gdbus-codegen）を host 用に作る。
- zedBSD guest での最小の実行確認（GMainLoop・GObject の型・gio の file 読み書き・GSettings の memory backend）。

## 受け入れ

3 package が build・install され、license audit を通る。guest で GLib の試験 program が期待どおりに動く（SSH/serial の操作と結果、`boot-test.sh` の起動確認）。

## 所有 path

`userland/packages/libs/{libffi,pcre2,glib}/`、`plan/ws115/`

## Standards / evidence / resume

[Guardrail](../../guardrail.md)、[C全文](../../coding-style.md)、[scoped方針](../../standards/ws114-gtk-qt-learning.md)、[automation](../../standards/automation.md)、[license audit](../../tools/packages/audit-licenses.sh) を実装前に確認する。外部 package は公式 tarball を取得・検証して patch し、source tree へ取り込まない（AGENTS.md）。新コードは全文規約。toolchain（`toolchain/`・`lang/clang`・`devel/libcxx`・共有 `build/llvm` 等）は変更・build しない。共有 `build/` は読取専用、自分の worktree の `build/` を使う。全体の `make check` は禁止。具体コマンド/版/結果/commit/環境/成果物/skip/制限は未実施（計画のみ）。各PhaseはQueueの個別scope承認後に実行する。前提未達・判断待ちならattemptをunclearedとして証拠/再開条件を残す。

Event ws115-beta1-plan-20261002: 2026-10-02 計画担当が依存 package の移植 Phase として作成。版・option は p001 の移植契約で確定する。planning/Queue none。GitHub publication pending。
