<!-- awesome-plan project=zedbsd record=ws005-p020 -->
# ws005-p020: RTL8822BU の USB passthrough でベータ1 の desktop の通し

Status: planning（p019 と、Archer の置き場所・試験用 AP の確認を待つ）
Disposition: normal
Parent: [WS005](../ws.md)
Focused goal: fg019（ベータ1）
Queue: none
目安: 2〜3h

## 範囲

TP-Link Archer T3U Nano（RTL8822BU、`2357:012e`）または T3U Plus（`2357:0138`）を QEMU の xHCI に USB passthrough し（`plan/ws004/tests/run-rtl8822bu-passthrough.py` を使う／直す）、
現在の main（p019 を含む）の image で:

1. CLI の回帰: `net wifi set-key`・`enable`・`list`・`connect`・DHCP・`fetch`・`disconnect`・`disable`（ws005-p009/p008 の流れ、2026-09 以降の変更で壊れていないか）。
2. desktop の B1〜B4: graphical login の後、Settings と system bar から on・scan・鍵の入力・接続・`fetch`、guest の再起動の後の再接続、off・鍵の誤り・AP の不在の表示。
   有線の usb-net を同時に付けて B3。
3. 見つかった不具合は、WS005 の範囲（networkd・net・libkeiland の network）なら直し、driver なら WS004 へ、画面なら WS089/WS035 へ Bug として返す。

鍵は実行時にだけ渡し、image・log・plan・commit に残さない（ws005-p009 の Credential handling）。鍵を入れた guest の disk は試験の後に消す。

## 受け入れ

- 1 と 2 の各項目の PASS/FAIL を QMP の `screendump` の PNG と guest の disk の log（console/serial log は使わない）で記録し、PNG をユーザーに見せる。
- QEMU（passthrough）の証拠であって実機の証拠ではないと書く。

## 検証

上記の通しと、修正したときはその領域の host の試験と `plan/tools/boot-test.sh`。

## 所有 path

`plan/ws005/phase020/`、`plan/ws005/tests/`、修正するときは p019 と同じ source。`plan/ws004/tests/run-rtl8822bu-passthrough.py` を直す必要があれば main に依頼する。

## 依存

p019。ユーザーの確認: Archer がどの host（centris か 5330 か）に挿してあるか、試験用 AP の SSID と鍵を実行時に渡す方法。

## 未決の判断

上記のユーザーの確認。
