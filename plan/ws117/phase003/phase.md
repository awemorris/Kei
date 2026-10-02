<!-- awesome-plan project=zedbsd record=ws117-p003 -->

# ws117-p003: 採用行の compositor 改良 その1（起動・window・装飾・入力）

Parent: [WS117](../ws.md)
Status: planning（p002 の採否待ち）
Disposition: normal
Primary Milestone: MG006（WSから継承）
Queue / attempts: none
Purpose / goal: p002 で採用された行のうち、素の Qt6 app の起動・window・装飾・入力を妨げる不足を Linux Keiland で直す。
Prerequisites: p002 の採用範囲。WS114 p007 cleared。WS114 p003 と同じ file を触るので直列（main が順を決める）。
Investigation bound: timebox 3〜4h。超える場合は p004 へ分ける。

## 範囲（p002 で確定）

見込み: xdg-decoration で Qt が server_side を要求した時の Keiland SSD の振る舞い（p007 の mode の上で）、Qt の popup/positioner の制約、text-input-v3 の Qt の使い方の差、cursor-shape、clipboard の mime の差。対象 file は `userland/desktop/wayland/` の protocol.c・toplevel.c・seat.c・popup.c・decoration.c・text-input.c・data.c と必要な header。共通 OS 境界と libvulkan 経由の GPU 契約を守る。HAL・toolchain・portal は範囲外。

## 受け入れ

採用された各行が代表 app で通る（PNG・trace）。WS114 の標準 GTK4 の smoke（GL 1 回）と native Terminal/Files の SSD に回帰が無い。zedBSD target build warning 0、`boot-test.sh` の PNG。変更 C の全文規約。

## 所有 path

採用行に必要な `userland/desktop/wayland/` の file（Queue の承認時に列挙）、`plan/ws117/`。

## Standards / resource

[Guardrail](../../guardrail.md)、[C全文規約](../../coding-style.md)、[automation](../../standards/automation.md)、[WS114–117 方針](../../standards/ws114-gtk-qt-learning.md) を変更前に実内容で読む。新しい C は全文規約。Linux guest は loopback SSH と QMP PNG（AGENTS.md の WS105/WS108 の例外）、serial/console log を判定に使わない。zedBSD の起動は `plan/tools/boot-test.sh` だけ。共有 toolchain/sysroot と main の `build/` は読取専用。QEMU と SSH port は main が割り当てる。全体の `make check` は禁止。具体コマンド/版/結果/commit/環境/成果物/skip/制限は実行時に記録する（今は計画のみ）。

2026-10-02 / ws117-beta1-plan-20261002: 計画担当が作成。scope は p002 後に確定。
