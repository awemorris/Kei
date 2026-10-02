<!-- awesome-plan project=zedbsd record=ws005-p018 -->
# ws005-p018: ベータ1 の WiFi の利用者の流れの調査と契約

Status: planned
Disposition: normal
Parent: [WS005](../ws.md)
Focused goal: fg019（ベータ1）
Queue: none（未承認）
目安: 2〜3h（調査と設計だけ、実装はしない）

## 範囲

現在の source で、ログインした利用者が WiFi を使い始めるまでの流れを end-to-end で読み、ベータ1 の受け入れ B1〜B4（[ws.md](../ws.md)）を満たすのに
足りない点を洗い出し、契約（状態・所有者・操作の順）を決める案を書く。調べる点:

1. **方針の所有者**: `networking` の oneshot（`net startup`、root）は root の `/etc/wifi.conf` で `net wifi enable` を行い、方針の所有者が root になる。
   Settings の鍵つき join（`userland/desktop/settings/network.c` の `se_network_join_key`）は利用者の `~/.wifi.conf` に鍵を書き、`profiles-changed` の後に
   `connect` を送るが、`connect` は所有者の store の profile を要る（`userland/base/networkd/main.c` の `load_policy(managed_wlan.owner_uid, ...)`）。
   **source を読んだ限りでは、所有者が root のままだと利用者の鍵で join できない疑いがある（未検証）**。偽の networkd を使った ws035-p013/ws089-p003 の試験はこれを捉えない。
2. 再起動の後の再接続（B2）: 利用者の鍵は login の後にしか使えない。案 A: session（sessiond または system bar）が login 時に利用者として `enable` する。
   案 B: Settings の join が system の store（`/etc/wifi.conf`）に書く（`network` group の権限の扱い）。案 C: 両方の store を読む。利点・権限・秘密の扱いを比べる。
3. system bar（`userland/desktop/wayland/network.c`）と Settings が同じ状態・同じ操作の順を使うか。WiFi の on/off、切断、鍵の誤り、AP の不在の表示（B4）。
4. 有線（WS033 の `managed-lan`）と WiFi が同時のときの default route・resolver の持ち主（B3）。`dhcpc` の子が resolver を上書きする順。
5. AX211 と RTL8822BU が同時にあるときの `wlanN` の選択（stable order）が利用者に見える形。
6. QEMU で本物の radio を使う試験の方法（`plan/ws004/tests/run-rtl8822bu-passthrough.py`、`run-intel-ax211-vfio-qemu.sh`）と、鍵を image・log・plan に残さない手順（ws005-p009 の Credential handling）。

## 受け入れ

- `plan/ws005/phase018/flow.md` に: 現在の流れ（source の file:行つき）、B1〜B4 ごとの足りない点、案の比較と推奨、p019 の実装の範囲（変更する file と試験）。
- ユーザーの判断が要る点（少なくとも 2 の案の選択）を一覧にして main へ返す。疑い（1）は host の試験または QEMU（偽の networkd でなく本物の networkd と `wifi` の偽の子）で
  確かめられれば確かめ、確かめられなければ「未検証」と書く。
- source は変えない。

## 検証

- 読んだ source の範囲、走らせた host の試験（あれば）、未実施の確認を記録する。

## 所有 path

`plan/ws005/phase018/`。読むだけ: `userland/base/net/`、`userland/base/networkd/`、`userland/desktop/libkeiland/zedbsd/network-*.c`、`userland/desktop/settings/`、
`userland/desktop/wayland/network.c`、`userland/desktop/sessiond/`。

## 依存

なし（BUG-134 の解決を待たない）。

## 未決の判断

- 2 の案（利用者の鍵の扱いと login 後の自動の再接続）はユーザーの判断。このPhaseは選択肢と推奨を出すまで。
