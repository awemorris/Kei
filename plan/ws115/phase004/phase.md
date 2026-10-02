<!-- awesome-plan project=zedbsd record=ws115-p004 -->

# ws115-p004: meson のクロス契約と host 道具

Parent: [WS115](../ws.md)
Status: planning
Disposition: normal
Primary Milestone: MG002（WSから継承）
Queue / attempts: none
Purpose / goal: GTK4 とその依存を meson で zedBSD target へ cross build できるようにする。
Prerequisites: p001 の契約、共有 path（external.mk・gen-cross-toolchain.sh）の main の割当
Investigation bound: timebox 4h / 1 Queue
Origin: WS034 p025（移管を main に依頼）

## 範囲

- `gen-cross-toolchain.sh` に meson の cross file（compiler wrapper・sysroot・pkg-config・`host_machine` の system 名）を生成させ、`external.mk` に meson/ninja で build する package の共通の規則を足す。CMake/autoconf の既存の契約と同じ wrapper を使う。
- pkg-config の wrapper（sysroot と package の prefix）、libtool の共有ライブラリの問題（inventory §3.2）、symbol versioning の無効化の共通の対応。
- host 道具: p001 で要ると決めたもの（meson の版、gperf の source build、native の glib 道具は p005 で glib と同じ版を作る、wayland-scanner）を `build/` の下の host tools として作る規則。
- 試験: 小さな meson の試験 package（例: graphene を先に通すか、試験用の 1 file の library）で cross file が通ることを確かめる。

## 受け入れ

meson 構成の小さな package が target 向けに build され、ELF の machine・interpreter・NEEDED が正しい。CMake/autoconf の既存 package（zlib・expat・curl）の build に回帰が無い。

## 所有 path

`userland/packages/external.mk`・`userland/packages/tools/`（共有、main の割当が要る）、`plan/ws115/`

## Standards / evidence / resume

[Guardrail](../../guardrail.md)、[C全文](../../coding-style.md)、[scoped方針](../../standards/ws114-gtk-qt-learning.md)、[automation](../../standards/automation.md)、[license audit](../../tools/packages/audit-licenses.sh) を実装前に確認する。外部 package は公式 tarball を取得・検証して patch し、source tree へ取り込まない（AGENTS.md）。新コードは全文規約。toolchain（`toolchain/`・`lang/clang`・`devel/libcxx`・共有 `build/llvm` 等）は変更・build しない。共有 `build/` は読取専用、自分の worktree の `build/` を使う。全体の `make check` は禁止。具体コマンド/版/結果/commit/環境/成果物/skip/制限は未実施（計画のみ）。各PhaseはQueueの個別scope承認後に実行する。前提未達・判断待ちならattemptをunclearedとして証拠/再開条件を残す。

Event ws115-beta1-plan-20261002: 2026-10-02 計画担当が依存 package の移植 Phase として作成。版・option は p001 の移植契約で確定する。planning/Queue none。GitHub publication pending。
