<!-- awesome-plan project=zedbsd record=ws005-p021 -->
# ws005-p021: AX211 の PCI passthrough でベータ1 の desktop の通し

Status: planning（BUG-134 の解決を待つ）
Disposition: normal
Parent: [WS005](../ws.md)
Focused goal: fg019（ベータ1）
Queue: none
目安: 2〜3h

## 範囲

5330 の AX211（`0000:00:14.3`）を vfio-pci で QEMU に渡し（`plan/ws004/tests/run-intel-ax211-vfio-qemu.sh`、ws004-p051 が直した手順）、iGPU の passthrough と組み合わせて
（`plan/tools/hw5330/README.md`）、p020 の 2 と同じ desktop の B1〜B4 を通す。RTL8822BU と AX211 の両方があるときの選択も 1 回確かめる。
5330 の host の制約（ws004-p051 の「host の制約」: 付け替えるのは `0000:00:14.3` だけ、終わったら iwlwifi に戻す、`/tmp/i915-hw.lock`）に従う。

## 受け入れ

- B1〜B4 の各項目の PASS/FAIL を PNG（`*-live.png`）と guest の disk の log で記録し、PNG をユーザーに見せる。passthrough の証拠であって単独起動の証拠ではないと書く。
- driver の不具合は WS004（BUG-134 の ticket は P1 の担当なので、新しい症状は main に報告して ticket を分けてもらう）。

## 検証

上記と、修正したときはその領域の host の試験。

## 所有 path

`plan/ws005/phase021/`、`plan/ws005/tests/`、修正するときは p019 と同じ source。

## 依存

[BUG-134](../../bugs/BUG-134.md) の解決（[ws004-p051](../../ws004/phase051/phase.md) の clearance と main への統合）、p019。p020 と同じ試験の手順を使うので p020 の後が望ましい。

## 未決の判断

なし（BUG-134 の結果で範囲が変わるなら planning に戻す）。
