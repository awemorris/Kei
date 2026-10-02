<!-- awesome-plan project=zedbsd record=ws095-p012 -->

# ws095-p012: 補いの辞書の拡張と活用の種類の注釈

Status: planned（p005 と並行可、compositor を触らない）
Disposition: normal
Parent: [WS095](../ws.md)
Queue: none
Prerequisites: p003・p004（cleared）
Investigation bound: timebox 4h

## 範囲

- `userland/desktop/ime/dict/SKK-JISYO.kei`（書き下ろし、zlib、D3）を千語へ広げ、候補に活用の種類の注釈（SKK の `;…`）を足す。`ja-dict.c`・`ja-segment.c` が注釈を読み、送り仮名の活用の候補の順を改善。
- held-out の文 100 以上を書き下ろし（`plan/ws095/tests/`）、拡張の前後で `measure.sh` の正解率を測る。

## 受け入れ

host の engine の試験が通り、held-out の正解率が拡張の前より下がらない（前後の数を記録）。既存の 100 文の正解率も下がらない。

## 所有 path

`userland/desktop/ime/`（dict・ja-dict.c・ja-segment.c）、`plan/ws095/tests/`

## 未決の判断

なし（D3 の答え 2026-09-29 夜に従う）

## Standards / evidence

[Guardrail](../../guardrail.md)、[C全文規約](../../coding-style.md)、[design.md](../design.md) を変更前に読む。新しい C は全文規約。zedBSD の起動は `plan/tools/boot-test.sh` だけ、guest の操作は serial/SSH、画面は PNG で目視しユーザーに見せる。serial/console log を判定に使わない。共有 `build/` と toolchain は読取専用。全体の `make check` は禁止。QEMU の証拠と実機の証拠を分け、やっていない確認は未実施と書く。具体コマンド/結果は実行時に記録する（今は計画のみ）。

2026-10-02 / ws095-beta1-plan-20261002: 計画担当が作成。
