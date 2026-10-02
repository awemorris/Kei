<!-- awesome-plan project=zedbsd record=ws118-p004 -->
# ws118-p004: 5320 のベータ1 の受け入れ（ユーザーと一緒に）

Status: planning（p003、ユーザーの時期）
Disposition: normal
Parent: [WS118](../ws.md)
Focused goal: fg019（ベータ1）
Queue: none
目安: 1〜2h（立会い）

## 範囲

WS129 の release candidate（またはその時点の main の、`plan/tools/boot-test.sh` を通した USB 用の image）で、5320 を USB から単独で起動し、ws.md の T3（内蔵 LCD の graphical login・
デスクトップ・keyboard/touchpad・App Home から app）と T4（USB の LAN の DHCP と `fetch`、無線は p002 で決めた手段）をユーザーに確かめてもらう。手順書を先に `plan/ws118/phase004/checklist.md` に作る。
ws005-p023・ws033-p002・WS129 の実機の確認と同じ日にまとめられる。

## 受け入れ

ユーザーの報告（実機の証拠）。不合格は Bug にして WS129 の既知の問題へ渡す。

## 所有 path

`plan/ws118/`。

## 依存

p003、ユーザーの時期。

## 未決の判断

なし。
