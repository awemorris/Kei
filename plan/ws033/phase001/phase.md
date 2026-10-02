<!-- awesome-plan project=zedbsd record=ws033-p001 -->
# ws033-p001: USB の LAN の後挿し・抜去・carrier の変化を QEMU で通す

Status: uncleared（q598-i01、P1、2026-10-02 Q1 の優先度の変更で中断。後で再投入）
Disposition: normal
Parent: [WS033](../ws.md)
Focused goal: fg019（ベータ1）
Queue: q598 / q598-i01（P1、中断）
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

## q598-i01 の途中の結果（P1、2026-10-02、base `0e9809833`、中断）

- 器: `plan/tools/guest/guest.sh`（SSH の guest、ue0 は管理用の usb-net、`GUEST_RUNTIME=build/p1-gr`）と、image
  `make ZEDBSD_CONFIG=plan/tools/guest/config-amd64-ssh.mk BUILD=build/p1-ssh $(guest.py extra-files) disk-image`（rc=0）。
  試験の usb-net は `--qemu-extra "-netdev user,id=net1,net=10.0.5.0/24,host=10.0.5.2,dhcpstart=10.0.5.15"` で netdev だけ用意し、QMP で後挿し。
  判定は SSH の `net show`・`route`・`ifconfig`（console/serial log は使っていない）。
- 起動の後: `ue0 static online`、route は `10.0.2.0/24` と `default 10.0.2.2 ue0`、`/etc/resolv.conf` は dhcpc（ue0）、`service status networking` は completed。
- **L1（後挿し）の観測: FAIL の疑い**。QMP `device_add usb-net,bus=xhci.0,port=4,id=hot,netdev=net1,mac=52:54:00:33:00:05` の後、
  `ue1` は 5 秒以内に生えて `ue1 static online` と表示されたが、30 秒の間 route は `169.254.0.0/16 link UC ue1` だけで、10.0.5.x の DHCP の address を
  得なかった（link-local への後退。WS033 の設計の「DHCP が取れなければ MAC から 169.254.x.y」）。その間に一度、SSH が `Connection timed out during
  banner exchange`。原因（後挿しの DHCP の timeout 10 秒の間に link/carrier が上がっていない、slirp の DHCP の応答、再試行の有無）は未調査。
- 止める直前の `ifconfig ue1`: `flags=UP,RUNNING`、`inet 169.254.1.0`、**RX packets 0**・TX packets 0（後挿しの ue1 は一つも送受信していない。
  DHCP の前の段、CDC ECM の data interface か bulk の開始を疑う、未調査）。
- L1 の fetch・抜去（L1/L2 の 2）・`set_link`（3）・`networking.wait`（4）は未実施。`lan-hotplug.sh` は未作成。
- 再開の手順: 同じ器で、`device_add` の後に `ifconfig ue1`（flags の RUNNING）と `net dhcp ue1 --timeout=20` を手で試し、managed-lan の後挿しの経路
  （RTM_IFINFO → PENDING → dhcp の timeout → 169.254）を読む。
