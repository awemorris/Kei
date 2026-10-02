<!-- awesome-plan project=zedbsd record=ws005-p023 -->
# ws005-p023: ベータ1 の network の実機の受け入れ（ユーザーと一緒に）

Status: planning（ユーザーの時期、p020〜p022）
Disposition: normal
Parent: [WS005](../ws.md)
Focused goal: fg019（ベータ1）
Queue: none
目安: 2h（ユーザーの立会い）

## 範囲

受け入れ B5: USB から単独で起動した実機で、

- 5330: AX211 で B1（Settings で鍵を入れて接続、`fetch`）と B2（再起動の後の再接続）。USB の LAN（RTL8156）の DHCP と `fetch`、抜いて WiFi で続くこと（B3）。
- 5320: USB の LAN の DHCP と `fetch`。無線は WS118 p002 の調査の結果で決める（内蔵の無線が未対応なら USB の Archer T3U で B1）。

Q1 はユーザーに時期を確かめてから始める。image は WS129 の release candidate か、その時点の main の image（`plan/tools/boot-test.sh` を通したもの）。
確認の手順書（ユーザーが行う操作と見るもの）を `plan/ws005/phase023/checklist.md` に先に用意する。

## 受け入れ

ユーザーの報告（目視・写真）を記録する。実機の証拠として QEMU の証拠と分けて書く。不合格の項目は Bug にして WS129 の既知の問題の一覧へ渡す。

## 所有 path

`plan/ws005/phase023/`。

## 依存

p020・p021・p022、ユーザーの時期。WS129 の hardware の確認の Phase と同じ日にまとめられる。

## 未決の判断

5320 の無線の手段（WS118 p002 の結果）。
