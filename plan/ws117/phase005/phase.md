<!-- awesome-plan project=zedbsd record=ws117-p005 -->

# ws117-p005: 素の Qt6 の再検証と WS115/WS116 への引継ぎ

Parent: [WS117](../ws.md)
Status: planning
Disposition: normal
Primary Milestone: MG006（WSから継承）
Queue / attempts: none
Purpose / goal: 最終 source の compositor で代表 3 app を再試験し、Qt6 の protocol・buffer・renderer・入力・依存/サービス・回避策を WS116（zedBSD 移植）と WS096（独自実装）へ、共通の部分を WS115 へ渡す。
Prerequisites: p003（と採用時の p004）。
Investigation bound: 2〜3h。

## 受け入れ

機能表の採用行が最終 source で再現する証拠、引継ぎ表（`plan/ws117/handoff.md`）、WS116 p001 で使う「zedBSD に要る Qt6 module と依存」の候補。

## Standards / resource

[Guardrail](../../guardrail.md)、[C全文規約](../../coding-style.md)、[automation](../../standards/automation.md)、[WS114–117 方針](../../standards/ws114-gtk-qt-learning.md) を変更前に実内容で読む。新しい C は全文規約。Linux guest は loopback SSH と QMP PNG（AGENTS.md の WS105/WS108 の例外）、serial/console log を判定に使わない。zedBSD の起動は `plan/tools/boot-test.sh` だけ。共有 toolchain/sysroot と main の `build/` は読取専用。QEMU と SSH port は main が割り当てる。全体の `make check` は禁止。具体コマンド/版/結果/commit/環境/成果物/skip/制限は実行時に記録する（今は計画のみ）。

2026-10-02 / ws117-beta1-plan-20261002: 計画担当が作成。
