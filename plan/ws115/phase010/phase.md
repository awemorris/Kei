<!-- awesome-plan project=zedbsd record=ws115-p010 -->

# ws115-p010: zedBSD QEMU で GTK4 demo app の起動と代表操作

Parent: [WS115](../ws.md)
Status: planning
Disposition: normal
Primary Milestone: MG002（WSから継承）
Queue / attempts: none
Purpose / goal: zedBSD 上の Keiland で upstream GTK4 の demo app を動かし、代表操作と renderer を確かめる。
Prerequisites: p002（GTK4 本体）、WS117 p003（compositor の改良）と WS114 p007（CSD）
Investigation bound: timebox 4h。起動しない場合は原因の切り分けと残件を返す
Origin: 2026-10-02 p002 から分割

## 範囲

- p001 で決めた demo app を zedBSD amd64 QEMU の Keiland で起動（`GDK_BACKEND=wayland`、最初は `GSK_RENDERER=cairo`）。window・CSD・click・key・menu・dialog・resize・clipboard を操作し、QMP PNG で確認。
- 余力があれば GL（zedBSD の libegl/libglesv2）と Vulkan（Venus）の renderer を各 1 回試し、成功と fallback を分ける。
- Linux の標準 GTK4（WS114）との差を表にする。

## 受け入れ

demo app が zedBSD QEMU で window を表示し、p001 の代表操作が通る PNG と操作の証拠。renderer の実際の種類を記録。`boot-test.sh` の起動確認。実機は未実施と書く（実機の確認はユーザーに依頼）。

## 所有 path

`userland/packages/desktop/gtk4/`（修正が要る場合の patch）、`plan/ws115/`。compositor の修正が要る場合は WS117/WS114 の Phase か main に依頼する

## Standards / evidence / resume

[Guardrail](../../guardrail.md)、[C全文](../../coding-style.md)、[scoped方針](../../standards/ws114-gtk-qt-learning.md)、[automation](../../standards/automation.md)、[license audit](../../tools/packages/audit-licenses.sh) を実装前に確認する。外部 package は公式 tarball を取得・検証して patch し、source tree へ取り込まない（AGENTS.md）。新コードは全文規約。toolchain（`toolchain/`・`lang/clang`・`devel/libcxx`・共有 `build/llvm` 等）は変更・build しない。共有 `build/` は読取専用、自分の worktree の `build/` を使う。全体の `make check` は禁止。具体コマンド/版/結果/commit/環境/成果物/skip/制限は未実施（計画のみ）。各PhaseはQueueの個別scope承認後に実行する。前提未達・判断待ちならattemptをunclearedとして証拠/再開条件を残す。

Event ws115-beta1-plan-20261002: 2026-10-02 計画担当が依存 package の移植 Phase として作成。版・option は p001 の移植契約で確定する。planning/Queue none。GitHub publication pending。


## 2026-10-02 user の追加

「GtkApplication が、session bus が無いと警告を出す件は、該当コードを無効化するパッチをお願いします。」→ この Phase の範囲に含める: GtkApplication（と下の GApplication の session bus の接続）が zedBSD で session bus を探さず警告を出さないよう、該当コードを `__ZEDBSD__` で無効化する patch を `userland/packages/desktop/gtk4/`（または glib 側なら glib の package）に置く。一意性（single instance）は無効のまま、app は通常どおり起動・終了する。gtk4-demo・widget-factory の起動で警告が出ないことを確かめる。
