<!-- awesome-plan project=zedbsd record=ws004-p051 -->
# ws004-p051: BUG-134 AX211 の起動停止と不動作を passthrough で解析・修正

Status: in-progress（q590-i01）
Disposition: normal
Parent: [WS004](../ws.md)
Bug: [BUG-134](../../bugs/BUG-134.md)
Queue: q590 / q590-i01（P1）

## 範囲

1. 5330 host（`solaris10-man`）で AX211（`0000:00:14.3`）を iwlwifi から vfio-pci へ一時的に付け替え、`CONFIG_DRIVER_PCI_INTEL_AX211 := y` の image を passthrough の QEMU で起動し、起動停止を再現する。既存の `plan/ws004/tests/run-intel-ax211-vfio-qemu.sh` を使う／直す。
2. 停止を QEMU の gdbstub・monitor・QMP で解析する（console/serial log で判定しない）。いつから壊れたか（git の履歴、ws004-p038 系の以前の到達点）も調べる。
3. 原因が `src/drivers/wifi/intel-ax211/`・firmware の package・その driver が使う kernel の部分にあれば修正する。HAL の API（`include/hal/hal.h`）の変更が要るなら差分を plan に置いて止める。
4. 修正後: driver 有効の image が passthrough で起動して login に至り、AX211 の device が attach する（scan・接続まで確かめられれば記録、無理なら未実施と書く）。driver 有効の image で `plan/tools/boot-test.sh`（passthrough 無し）。build warning 0。新しい code は C 全文規約。

## host の制約

- 付け替えるのは `0000:00:14.3` だけ。終わったら iwlwifi へ戻す。host の USB の LAN、iGPU の mode、他の QEMU/VM、host の reboot・kernel・package には触れない。
- 5330 host の試験は `/tmp/i915-hw.lock`（centris）を flock で取ってから行う。

## 受け入れ

- 起動停止の原因の特定（証拠つき）と修正、上の 4 の確認。実機の単独起動（USB）はユーザーの確認で、未実施と書く。

## 結果

（q590 の結果を書く）
