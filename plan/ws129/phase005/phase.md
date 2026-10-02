<!-- awesome-plan project=zedbsd record=ws129-p005 -->
# ws129-p005: release notes・既知の問題の一覧・利用の手引き

Status: planning（p001 と各 WS の成果を待つ、10/13 頃に集める）
Disposition: normal
Parent: [WS129](../ws.md)
Focused goal: fg019（ベータ1）
Queue: none
目安: 2〜3h

## 範囲

1. release notes: 新機能（fg019 の各 WS の到達を ws.md から集める。到達していないものは書かない）、対象 platform（Latitude 5330・5320、各々の制限）、動作の確認の範囲（QEMU と実機を分ける）。
2. 既知の問題の一覧: [Bug Board](../../known-bugs.md) の open の行から、利用者に見えるものを選び、回避の方法を添える。WS の実機の Phase（ws005-p023・ws118-p004・ws119-p006）の不合格を加える。
3. 利用の手引き: USB への書き方（img.gz の展開、Windows・Linux・macOS）、BIOS の設定（UEFI の boot、Secure Boot を切る要否）、初回の login、WiFi の接続（Settings）、使える USB の LAN・WiFi の adapter、インストーラ（WS119 が載れば）。
4. 置き場所は p001 で決めた所（例 `docs/release/beta1.md`）。CI の本文はその file から作る。

## 受け入れ

文書の草稿を main とユーザーに review してもらう（ユーザーの確認が要る）。

## 所有 path

p001 で決めた文書の path、`plan/ws129/`。

## 依存

p001、各 WS の成果。

## 未決の判断

なし（内容の review はユーザー）。
