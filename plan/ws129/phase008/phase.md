<!-- awesome-plan project=zedbsd record=ws129-p008 -->
# ws129-p008: ベータ1 の公開（ユーザーの指示でだけ）

Status: planning（p007 とユーザーの公開の指示を待つ）
Disposition: normal
Parent: [WS129](../ws.md)
Focused goal: fg019（ベータ1）
Queue: none
目安: 1h

## 範囲

ユーザーの明示の指示の後でだけ: tag の作成と push（またはユーザーの操作）、CI の release の job の実行の確認、公開された配布物の download と SHA-256 の照合、release notes の表示の確認。
AGENTS.md の「push はしない」はユーザーの指示で上書きされる範囲に限る（指示の文言と範囲を記録する）。

## 受け入れ

公開の URL、配布物の SHA-256 の一致、release の本文の確認。

## 所有 path

`plan/ws129/`。

## 依存

p007、ユーザーの公開の指示。

## 未決の判断

公開の指示そのもの。
