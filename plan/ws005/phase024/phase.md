<!-- awesome-plan project=zedbsd record=ws005-p024 -->
# ws005-p024: WiFi の有効化（起動時の `net wifi enable`）で保存済みの AP に自動で再接続する

Status: planned
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
