<!-- awesome-plan project=zedbsd record=ws117-p004 -->

# ws117-p004: 採用行の compositor 改良 その2（任意の protocol）

Parent: [WS117](../ws.md)
Status: planning（p002 の採否待ち。採用が無ければ取消）
Disposition: normal
Primary Milestone: MG006（WSから継承）
Queue / attempts: none
Purpose / goal: p003 に入らない採用行（例: xdg-activation、fractional-scale、primary selection の転送、D&D）を実装する。
Prerequisites: p002、p003 cleared。
Investigation bound: timebox 3〜4h。

## 受け入れ

採用された行が代表 Qt6 app と標準 GTK4 の両方で通る（両方が使う protocol の場合）。回帰と全文規約は p003 と同じ。

## Standards / resource

[Guardrail](../../guardrail.md)、[C全文規約](../../coding-style.md)、[automation](../../standards/automation.md)、[WS114–117 方針](../../standards/ws114-gtk-qt-learning.md) を変更前に実内容で読む。新しい C は全文規約。Linux guest は loopback SSH と QMP PNG（AGENTS.md の WS105/WS108 の例外）、serial/console log を判定に使わない。zedBSD の起動は `plan/tools/boot-test.sh` だけ。共有 toolchain/sysroot と main の `build/` は読取専用。QEMU と SSH port は main が割り当てる。全体の `make check` は禁止。具体コマンド/版/結果/commit/環境/成果物/skip/制限は実行時に記録する（今は計画のみ）。

2026-10-02 / ws117-beta1-plan-20261002: 計画担当が作成。
