<!-- awesome-plan project=zedbsd record=ws005-p024 -->
# ws005-p024: WiFi の有効化（起動時の `net wifi enable`）で保存済みの AP に自動で再接続する

<<<<<<< HEAD
Status: planned（2026-10-02 ユーザーの改訂の設計）
=======
Status: uncleared（q607-i01、P1、2026-10-02。実装の最初の変更が権限の確認で拒否され、source は変えずに停止）
>>>>>>> d108d0a0d
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

<<<<<<< HEAD

## 2026-10-02 設計の改訂（ユーザー）

ユーザー:「ログインしていない利用者の鍵を接続に使うのはおかしいです。システムのストアに保存されるWiFi情報と、ユーザのストアに保存されるものを、分離して実装済みですよね。networkdには、ログイン後にユーザのストアの位置を通知して利用可能にするのがいいです。ログアウト時も通知が必要ですね。」

上の範囲 1・2（起動時に network group の全利用者の `~/.wifi.conf` を読む）は**取り消し**。改訂した範囲:
1. 起動時（`net startup`／`net wifi enable`、root）は **system の store だけ**で、見えている保存済みの AP に自動接続する（console の起動でも）。
2. 利用者の login（sessiond の session の開始）で、session が networkd に**その利用者の store の位置を通知**し、networkd はその利用者の store を候補に加えて、保存済みの AP に自動接続する（p019 の policy の移動の仕組みと整合）。
3. logout（session の終了）で、session が networkd に**通知**し、networkd はその利用者の store を候補から外す（その store の profile で張った接続は切り、system の store の候補へ戻る）。
4. 通知は既存の networkd の socket の protocol の要求として足す（network group の利用者だけ、要求者自身の store だけを登録できる）。鍵は利用者の store に残し、networkd に鍵を渡さない境界は不変。
5. 確認: 本物の networkd の QEMU（system の store だけでの起動時の接続の試み、login の通知での利用者の store の追加、logout での除去）、5330 の AX211 passthrough で再起動後の console の自動接続（system の store）と、login 後の利用者の store での自動接続、logout 後の切り替え。資格情報は記録に書かない。

2026-10-02 user（明示の承認）:「WiFi自動再接続を明示的に承認します。」→ 改訂した設計（起動時は system の store、login で利用者の store を通知して追加、logout で通知して除去）の実装と試験を承認。
=======
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
>>>>>>> d108d0a0d
