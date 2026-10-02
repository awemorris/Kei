<!-- awesome-plan project=zedbsd record=ws127-p008 -->

# ws127-p008: 全文規約と回帰（WS の最後）

Status: planning（2026-10-02 ベータ1の計画。要る判断・成果は「未決の判断」と「依存」）
Disposition: normal
Parent: [WS127](../ws.md)
Queue: なし
依存: 実装の Phase の全て
目安: 2h（1 Queue）。実行者の目安: phase-runner-mid
所有 path: WS127 で変えた source、`plan/ws127/`

## 範囲

WS127 で変えた全ての source を coding-style.md の全文で見直し、style-check と build（warning 0）、Files の回帰 14 本・host・Settings の host（共有 file を変えたとき）・WS094 の desktop の手順（`files-desktop-guest.sh install show input menu`）・boot test。

## 受け入れ

範囲の中の違反 0、全て PASS。WS の完了の判定を Q1 に依頼（試験は `plan/tools/files/` へ）。
変えた source の style-check の違反 0、build の warning 0、`files-regress.sh` の 14 本と boot test PASS。

## 未決の判断

なし

## 検証の方法と範囲

QEMU の Venus（`plan/tools/files/files-guest.sh`、`files-regress.sh`）と host の試験。console・serial の log では判定しない。各 Phase の最後に `files-regress.sh` の 14 本と boot test。
Settings と共有の `canvas.c`・`text.c`・`icons.c` を変えたら `sh plan/ws089/tests/host-build.sh` と Settings の host 試験も流す。

## Event

2026-10-02 / ws127-beta1-plan: fg019 の計画で新設（planning）。p001 の結果とユーザーの選択で範囲を確定し planned にする。
