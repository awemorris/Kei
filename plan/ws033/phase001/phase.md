<!-- awesome-plan project=zedbsd record=ws033-p001 -->
# ws033-p001: USB の LAN の後挿し・抜去・carrier の変化を QEMU で通す

Status: planned
Disposition: normal
Parent: [WS033](../ws.md)
Focused goal: fg019（ベータ1）
Queue: none（未承認）
目安: 2〜3h

## 範囲

既存の `plan/ws033/tests/ssh-host-to-guest.sh`（USB 起動・usb-net・host から guest への SSH）を土台に、QMP で次を行う試験 `plan/ws033/tests/lan-hotplug.sh` を作る:

1. usb-net を付けずに起動 → `device_add` で後挿し → `ue0` が生え、`net lan enable` の管理で DHCP の address（QEMU の user network の 10.0.2.x）を得る。guest の SSH と `fetch`（host の用意した HTTP）。
2. `device_del` で抜去 → address・route が外れ、networkd が止まらない → 再び `device_add` で取り直す。
3. QEMU の `set_link <netdev> off/on` が usb-net の CDC の `NETWORK_CONNECTION` 通知として guest の carrier に届くかを調べる。届くなら carrier down/up で
   address の撤去と取り直し（L2）、届かないなら「QEMU では carrier を動かせない」と記録して L2 は実機（p002）へ回す。
4. rc.conf の `networking.wait` を true にした image で、usb-net あり（address を得て抜ける）と無し（設定の時間で抜けて login に届く）を比べる（L3）。

見つかった不具合は `userland/base/networkd/managed-lan.c`・`userland/base/networkd/main.c` の有線の部分・`userland/base/net/main.c` の `lan`/`startup` で直し、
`plan/ws033/tests/managed-lan-host-test.c` に case を足す。guest の判定は SSH の応答で行い、console/serial log は使わない。

## 受け入れ

- 1〜4 の PASS/FAIL（3 は調査の結果）を記録。`make managed-lan-host-test` が通る。修正したら build warning 0、新しい code は全文規約、最後に `plan/tools/boot-test.sh`（PNG をユーザーに見せる）。

## 所有 path

`plan/ws033/`、修正するときは上の source。

## 依存

なし。ws005-p019 と source が重なるので同時に走らせない（main が順を決める）。

## 未決の判断

なし。
