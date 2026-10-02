<!-- awesome-plan project=zedbsd record=ws095-p007 -->

# ws095-p007: Text Editor と Notes の確認と不足の修正

Status: planned（p005 cleared の後）
Disposition: normal
Parent: [WS095](../ws.md)
Queue: none
Prerequisites: p005。Text Editor は既に `kui_window_text_input` を呼んでいる（textedit/main.c）
Investigation bound: timebox 2〜3h

## 範囲

- Text Editor（WS092）で preedit の表示・cursor の矩形・確定・取り消し・複数行を確認し、不足を直す。
- Notes（libkeiui の window）に text input を足す。
- Settings の検索の field が text input を求めるかを確認（求めない場合は記録だけ、修正は WS089 の担当へ）。

## 受け入れ

zedBSD QEMU で Text Editor と Notes に日本語を入力・確定・保存し、再読み込みで一致。PNG。既存の textedit の host 試験（`plan/tools/textedit/`）に回帰無し。

## 所有 path

`userland/desktop/textedit/`・`userland/desktop/notes/`、`plan/ws095/`

## 未決の判断

なし

## Standards / evidence

[Guardrail](../../guardrail.md)、[C全文規約](../../coding-style.md)、[design.md](../design.md) を変更前に読む。新しい C は全文規約。zedBSD の起動は `plan/tools/boot-test.sh` だけ、guest の操作は serial/SSH、画面は PNG で目視しユーザーに見せる。serial/console log を判定に使わない。共有 `build/` と toolchain は読取専用。全体の `make check` は禁止。QEMU の証拠と実機の証拠を分け、やっていない確認は未実施と書く。具体コマンド/結果は実行時に記録する（今は計画のみ）。

2026-10-02 / ws095-beta1-plan-20261002: 計画担当が作成。
