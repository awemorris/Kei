<!-- awesome-plan project=zedbsd record=ws033-p002 -->
# ws033-p002: 実機で USB の LAN の抜き差しを確かめる（ユーザーと一緒に）

Status: planning（ユーザーの時期と p001）
Disposition: normal
Parent: [WS033](../ws.md)
Focused goal: fg019（ベータ1）
Queue: none
目安: 1h（ユーザーの立会い）

## 範囲

USB から単独で起動した 5330 と 5320 で、RTL8156（CDC NCM）を (1) 起動後に挿す、(2) ケーブルを抜いて挿す、(3) adapter ごと抜いて挿す。各回 `net show` と `fetch` の結果、
system bar の表示をユーザーに見てもらう（または WS118 p001 の sshd の image で host から SSH して `net show` を取る）。手順書を `plan/ws033/phase002/checklist.md` に先に作る。
ws005-p023（network の実機の受け入れ）と同じ日にまとめる。

## 受け入れ

L4 の各回の結果をユーザーの報告として記録（実機の証拠）。不合格は Bug にする。

## 所有 path

`plan/ws033/phase002/`。

## 依存

p001、ユーザーの時期。

## 未決の判断

なし。
