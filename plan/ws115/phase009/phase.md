<!-- awesome-plan project=zedbsd record=ws115-p009 -->

# ws115-p009: libwayland-client の upstream ABI 互換・wayland-protocols・wayland-cursor

Parent: [WS115](../ws.md)
Status: planning
Disposition: normal
Primary Milestone: MG002（WSから継承）
Queue / attempts: none
Purpose / goal: upstream wayland-scanner の生成 code を使う GTK4（と Qt6）が zedBSD の libwayland-client で動くようにする。
Prerequisites: p001 の不足の一覧、共有 path（`userland/desktop/libwayland`）の main の割当
Investigation bound: timebox 4h
Origin: WS034 p034（移管を main に依頼）

## 範囲

- p001 で見つけた不足の関数（`wl_proxy_marshal_flags` 等）・`wayland-client-core.h` の header・`.pc` を zedBSD の独自 libwayland に足す（独立実装を保ち upstream の source を取り込まない、WS034 の 2026-09-23 決定）。wayland-cursor は独自実装か upstream の package か p001 の判断。
- wayland-protocols は data だけの package（`userland/packages/desktop/wayland-protocols`）。
- 既存の Keiland app・libvulkan の WSI・libwayland-egl に回帰を出さない。

## 受け入れ

upstream wayland-scanner で生成した xdg-shell の client code を zedBSD の libwayland でリンク・実行する小さな試験が guest の Keiland に window を出す。既存の native app（Terminal・Files）と Vulkan の demo に回帰無し、`boot-test.sh` の PNG。変更 C の全文規約。

## 所有 path

`userland/desktop/libwayland/`（共有、main の割当が要る）、`userland/packages/desktop/wayland-protocols/`、`plan/ws115/`

## Standards / evidence / resume

[Guardrail](../../guardrail.md)、[C全文](../../coding-style.md)、[scoped方針](../../standards/ws114-gtk-qt-learning.md)、[automation](../../standards/automation.md)、[license audit](../../tools/packages/audit-licenses.sh) を実装前に確認する。外部 package は公式 tarball を取得・検証して patch し、source tree へ取り込まない（AGENTS.md）。新コードは全文規約。toolchain（`toolchain/`・`lang/clang`・`devel/libcxx`・共有 `build/llvm` 等）は変更・build しない。共有 `build/` は読取専用、自分の worktree の `build/` を使う。全体の `make check` は禁止。具体コマンド/版/結果/commit/環境/成果物/skip/制限は未実施（計画のみ）。各PhaseはQueueの個別scope承認後に実行する。前提未達・判断待ちならattemptをunclearedとして証拠/再開条件を残す。

Event ws115-beta1-plan-20261002: 2026-10-02 計画担当が依存 package の移植 Phase として作成。版・option は p001 の移植契約で確定する。planning/Queue none。GitHub publication pending。
