<!-- awesome-plan project=zedbsd record=ws099-p022 -->

# ws099-p022: WS099 の全文規約と回帰（WS の最後）

Status: planning（最後の Phase。p019〜p021 と p012 の結果の後に planned へ）
Disposition: normal
Parent: [WS099](../ws.md)
Queue: なし
依存: p019・p020・p021 cleared、p012 の結果（実機の C1）。C6 の扱いのユーザーの判断（p006）
目安: 3h（1 Queue）。実行者の目安: phase-runner（high）
所有 path: WS099 で変えた source（`userland/desktop/wayland/`・`userland/desktop/sessiond/`・greeter の該当の所）、`plan/ws099/`

## 範囲

Awesome Plan §6 の code を作る WS の最後の conformance。[guide.md](../guide.md) の「提案 p018」をこの番号で正式にした。

1. WS099 の commit（p002・p005・p007・p009・p010・p011・p015・p016・p019・p020・p021）の変えた file の一覧を `git log` から作る。
2. [coding-style.md](../../coding-style.md) の全文で見直し、`plan/tools/style-check.py` と範囲の中の違反を直す。新しい機能は足さない。
3. build（warning 0）、`criteria.sh` の C1〜C5・C7〜C10（C10 は 1 時間）を同じ最終の image で、boot test。

## 受け入れ

- 範囲の中の style の違反 0（既存の例外は表に記録）、warning 0。
- `criteria.sh` の C1〜C5・C7〜C10 が全て PASS（ベータ1 の B5）。p076 を含む C9 が安定（BUG-125 resolved の後）。
- 実機の証拠（C1・C5・C10 の 5330）と QEMU の証拠を分けて記録し、C6 の値と扱い（ユーザーの判断）を書く。
- 満たせば WS099 の完了の判定を Q1 に依頼する（完了の書き直しは AGENTS.md の形）。

## 未決の判断

- C6（窓 10 個で中央値 50 ms、今 56〜59 ms）を WS099 の完了の条件に残すか（2026-09-30 の決定「10/10 に見直す」、ユーザー）。

## Event

2026-10-02 / ws099-beta1-plan-p022: fg019 の計画で新設（guide の提案 p018 を正式化）。
