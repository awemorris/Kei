<!-- awesome-plan project=zedbsd record=ws117-p006 -->

# ws117-p006: 全変更の全文規約と最終回帰

Parent: [WS117](../ws.md)
Status: planning
Disposition: normal
Primary Milestone: MG006（WSから継承）
Queue / attempts: none
Purpose / goal: WS117 が変えた全 source の C 全文規約、formatter/style-check、OS/GPU 境界 checker、Linux build、zedBSD target build（warning 0）、代表 Qt6 app と標準 GTK4 の guest 回帰、`boot-test.sh` を最終版で確認する。
Prerequisites: p005 の証拠と最終 source。
Investigation bound: 2〜3h。

## 受け入れ

必要な規約/回帰を通し、版/結果/例外/skip/制限を記録する。

## Standards / resource

[Guardrail](../../guardrail.md)、[C全文規約](../../coding-style.md)、[automation](../../standards/automation.md)、[WS114–117 方針](../../standards/ws114-gtk-qt-learning.md) を変更前に実内容で読む。新しい C は全文規約。Linux guest は loopback SSH と QMP PNG（AGENTS.md の WS105/WS108 の例外）、serial/console log を判定に使わない。zedBSD の起動は `plan/tools/boot-test.sh` だけ。共有 toolchain/sysroot と main の `build/` は読取専用。QEMU と SSH port は main が割り当てる。全体の `make check` は禁止。具体コマンド/版/結果/commit/環境/成果物/skip/制限は実行時に記録する（今は計画のみ）。

2026-10-02 / ws117-beta1-plan-20261002: 計画担当が作成。
