<!-- awesome-plan project=zedbsd record=ws117-p002 -->

# ws117-p002: Qt6 機能表の行別採否

Parent: [WS117](../ws.md)
Status: planning（p001 の表待ち）
Disposition: normal
Primary Milestone: MG006（WSから継承）
Queue / attempts: none
Purpose / goal: p001 の表の各行をユーザーが採用・限定採用・保留に分け、p003/p004 の scope と受け入れを確定する。
Prerequisites: p001 cleared。
Investigation bound: user の判断の席（目安 30 分）。WS114 p002 と同じ席を推奨（共通行: decoration・text-input・activation・primary selection・fractional scale）。

## 受け入れ

各行の判断の出典（user の発言）と、p003/p004 の具体 scope・受け入れ・順番が記録される。ユーザー未判断の行は採用しない。採用ゼロの p004 は取消と理由を残す。

## Standards / resource

[Guardrail](../../guardrail.md)、[C全文規約](../../coding-style.md)、[automation](../../standards/automation.md)、[WS114–117 方針](../../standards/ws114-gtk-qt-learning.md) を変更前に実内容で読む。新しい C は全文規約。Linux guest は loopback SSH と QMP PNG（AGENTS.md の WS105/WS108 の例外）、serial/console log を判定に使わない。zedBSD の起動は `plan/tools/boot-test.sh` だけ。共有 toolchain/sysroot と main の `build/` は読取専用。QEMU と SSH port は main が割り当てる。全体の `make check` は禁止。具体コマンド/版/結果/commit/環境/成果物/skip/制限は実行時に記録する（今は計画のみ）。

2026-10-02 / ws117-beta1-plan-20261002: 計画担当が作成。
