<!-- awesome-plan project=zedbsd record=ws119-p006 -->
# ws119-p006: 実機の導入（ユーザーと一緒に）

Status: planning（p005、ユーザーの時期と導入先の disk の判断を待つ）
Disposition: normal
Parent: [WS119](../ws.md)
Focused goal: fg019（ベータ1）
Queue: none
目安: 1〜2h（立会い）

## 範囲

ユーザーが決めた導入先の disk（5330 の内蔵 NVMe は Linux の host なので別の disk の案）に、ベータ1 の候補の USB の image から導入し、導入先から単独で起動して login する（受け入れ I4）。
手順書を先に `plan/ws119/phase006/checklist.md` に作る。5320 で行うかはユーザーの判断。

## 受け入れ

ユーザーの報告（実機の証拠）。不合格は Bug にして WS129 の既知の問題へ渡す。

## 所有 path

`plan/ws119/`。

## 依存

p005、ユーザーの時期、導入先の disk の判断。

## 未決の判断

導入先の disk。
