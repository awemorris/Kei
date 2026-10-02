<!-- awesome-plan project=zedbsd record=ws005-p024 -->
# ws005-p024: WiFi の有効化（起動時の `net wifi enable`）で保存済みの AP に自動で再接続する

Status: uncleared（q607-i01、P1、2026-10-02。実装の最初の変更が権限の確認で拒否され、source は変えずに停止）
Disposition: normal
Parent: [WS005](../ws.md)

## ユーザーの決定（2026-10-02）

「ログインしたときにWiFiを再接続してください。ログインというか、networkdが有効になって、net wifi enableされたときですね。コンソール起動でもWiFi接続は自動で行われます。」

## 範囲

1. networkd が WiFi の policy を有効にしたとき（起動時の `networking` service の `net startup`／`net wifi enable`、root）、root の store に加えて `network` group の利用者が保存した profile（各利用者の `~/.wifi.conf`）も候補として読み、見えている保存済みの AP へ自動で接続する。console の起動（desktop の login 無し）でも同じ。
2. 鍵は各利用者の store に残し、networkd（root）が接続のために読むだけ（新しい system 全体の鍵の store は作らない）。どの利用者の profile で接続したかを状態に出す。複数の候補は既存の優先の規則（見えている中で最後に使った・明示の優先度）で選ぶ。
3. ws005-p019 の explicit な join での policy の移動、有線優先（B3）と整合させる。
4. 確認: 本物の networkd の QEMU（profile を持つ kei の store、root の store が空でも起動後に接続を試みる）、5330 の AX211 passthrough で再起動後に desktop の login 無し（console）と login 後の両方で自動接続。boot-test。資格情報は記録に書かない。
5. permission system に止められたら、この決定の引用とともに止まって Q1 に返す。

## 所有 path

`userland/base/networkd/`、`userland/base/net/`、`plan/ws005/`。

## q607-i01（P1 generation2、2026-10-02、base `b6e5ac6ef`）: 権限の確認で停止

- 試みた変更（適用されていない）: `userland/base/networkd/main.c` の `load_policy()` を、所有者が root のとき root の store に加えて `network` group の
  member（group の database の `gr_mem`）の各 `~/.wifi.conf` を `wifi_store_load_for_user` で読み、まだ無い SSID の profile を model に足す形にする
  （root の store が無くても member の profile があれば候補にする）。どの store の profile で接続したかを覚えて状態の行に出す。
- 結果: この編集が auto mode の classifier に「Security Weaken」として拒否された。指示どおり、別の経路で同じ結果を作らずに止めた。source は変わっていない
  （`git status` が clean）。
- 引用（ユーザーの決定、Q1 経由で届いた文言）:「ログインしたときにWiFiを再接続してください。ログインというか、networkdが有効になって、net wifi enableされたときですね。コンソール起動でもWiFi接続は自動で行われます。」
- 再開の条件: ユーザー本人が、このセッションの権限の確認の場で、networkd（root）が network group の利用者の store を起動時の自動接続のために読む変更を
  明示に承認する（または permission の規則を足す）。設計は上のとおりで、続きは `wifi_profiles_changed` で root の policy のときに member の通知でも
  起こすこと、状態の行の `owner=`・`profile-owner=`、QEMU の本物の networkd の試験と 5330 の passthrough の試験。
