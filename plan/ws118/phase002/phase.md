<!-- awesome-plan project=zedbsd record=ws118-p002 -->
# ws118-p002: ユーザーと 5320 の実機で log を取り、LCD の制御の失敗を分類する

Status: planning（p001、ユーザーの時期、5320 の network の手段の確認を待つ）
Disposition: normal
Parent: [WS118](../ws.md)
Focused goal: fg019（ベータ1）
Queue: none
目安: 2h（ユーザーの立会い）＋解析 1h

## 範囲

Q1 はユーザーに時期を確かめてから始める。p001 の手順書に沿って:

1. C（i915 無し）で起動し SSH で inventory を取る: PCI（iGPU・無線・NVMe）、USB、ACPI の panel/backlight、OpRegion/VBT の取り出し（取れる手段があれば）、内蔵の無線の chip（WS004 への提案の材料）。
2. B（i915 あり、message を画面に）と A（本番の形）で起動し、ユーザーに画面を見てもらいながら SSH で `dmesg`・session の log を取る。hang したら p001 の退路で disk の log を読む。
3. 失敗を分類する（例: panel の電源・backlight の順、eDP の link training、VBT の解釈、firmware の画面の引き継ぎ、TGL 固有の register）。分類ごとに証拠と、修正の Phase の案（範囲・2〜4h の大きさ・試す方法）を書く。

## 受け入れ

- 取った log と inventory を `plan/ws118/phase002/`（大きいものは `plan/ws118/temp/`、git に入れない）に保存し、分類と p003 以降の Phase の案を書く。実機の証拠と書く。
- 内蔵の無線の ID を ws005 と WS004 への提案として main に返す。

## 所有 path

`plan/ws118/`。

## 依存

p001、ユーザーの時期、5320 の network の手段（RTL8156 か Archer か）の確認。

## 未決の判断

なし（p002 の結果で p003 以降を決める）。
