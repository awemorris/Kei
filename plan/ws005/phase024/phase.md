<!-- awesome-plan project=zedbsd record=ws005-p024 -->
# ws005-p024: WiFi の有効化（起動時の `net wifi enable`）で保存済みの AP に自動で再接続する

Status: cleared（2026-10-03 user の判断。鍵の要る試験と P3 の統合後の libkeiland の join の確認は未実施のまま [WS133](../../ws133/ws.md) へ移管。q631-i02〜i04 の結果は下）
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


## 2026-10-02 設計の改訂（ユーザー）

ユーザー:「ログインしていない利用者の鍵を接続に使うのはおかしいです。システムのストアに保存されるWiFi情報と、ユーザのストアに保存されるものを、分離して実装済みですよね。networkdには、ログイン後にユーザのストアの位置を通知して利用可能にするのがいいです。ログアウト時も通知が必要ですね。」

上の範囲 1・2（起動時に network group の全利用者の `~/.wifi.conf` を読む）は**取り消し**。改訂した範囲:
1. 起動時（`net startup`／`net wifi enable`、root）は **system の store だけ**で、見えている保存済みの AP に自動接続する（console の起動でも）。
2. 利用者の login（sessiond の session の開始）で、session が networkd に**その利用者の store の位置を通知**し、networkd はその利用者の store を候補に加えて、保存済みの AP に自動接続する（p019 の policy の移動の仕組みと整合）。
3. logout（session の終了）で、session が networkd に**通知**し、networkd はその利用者の store を候補から外す（その store の profile で張った接続は切り、system の store の候補へ戻る）。
4. 通知は既存の networkd の socket の protocol の要求として足す（network group の利用者だけ、要求者自身の store だけを登録できる）。鍵は利用者の store に残し、networkd に鍵を渡さない境界は不変。
5. 確認: 本物の networkd の QEMU（system の store だけでの起動時の接続の試み、login の通知での利用者の store の追加、logout での除去）、5330 の AX211 passthrough で再起動後の console の自動接続（system の store）と、login 後の利用者の store での自動接続、logout 後の切り替え。資格情報は記録に書かない。

2026-10-02 user（明示の承認）:「WiFi自動再接続を明示的に承認します。」→ 改訂した設計（起動時は system の store、login で利用者の store を通知して追加、logout で通知して除去）の実装と試験を承認。
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

## q612-i01 の結果（P1 generation3、2026-10-03、base main `192364b9b`、commit `12a3f93e0`・`9b2c53812`）

承認: このセッションのユーザー本人の発言「WiFi自動再接続を明示的に承認します。」と改訂の設計（上の節、Q1 の起動指示に引用）。

### 実装したこと

- protocol（`userland/base/net/protocol.h`）: `NETWORKD_OP_WIFI_SESSION_OPEN`（38）・`NETWORKD_OP_WIFI_SESSION_CLOSE`（39）と field
  `NETWORKD_FIELD_ACCOUNT`（33、uid の u32）。要求は account だけを運び、store の場所は networkd が account の database（passwd の home）から
  決める（client が送る path は受け取らない）。鍵はこの socket を通らない。
- networkd（`userland/base/networkd/main.c`）:
  - 開いている session の account の一覧（最大 8、開いた順）。root（sessiond）は任意の account を、`network` group の利用者は自分だけを
    open/close できる（他人を名指すと EPERM、database に無い account は ENOENT、group 外は socket が拒否）。
  - 自動接続の候補（`load_candidates`）: 開いている session の store（新しい順）→ policy の所有者の store → system の store（`/etc/wifi.conf`）。
    同じ SSID は先の store の鍵を使う。読めない store は飛ばして他を使う。どの store の profile で接続したかを記録し（`wifi_connection_store_uid`）、
    再接続（recover）はその store から鍵を読み直す。
  - 起動時（root の `net startup`）は session が無いので system の store だけ（設計 1）。login の open で候補に加えて探索をやり直す（設計 2）。
    logout の close で候補から外し、その account が join で policy を持っていれば root に戻し（接続を切って system の store で探索）、root の
    policy でもその account の store の鍵で張った接続なら切って探索に戻る（設計 3）。
  - 利用者の `PROFILES_CHANGED` は、その store が候補（所有者・開いている session・root）の時だけ探索を起こす。
  - 状態の行に `owner=UID store=UID|- sessions=UID,...|-` を足した（desktop の parser は key で読むので影響無し）。
- `net wifi session open|close [account]`（`userland/base/net/main.c`）。
- sessiond（`userland/desktop/sessiond/session.c`。起動指示の `userland/base/sessiond/` は存在せず、同じ部品の実体はこの path）: session の
  開始の後に `net wifi session open UID`、終了（sweep の後）に `close UID` を、二重 fork の background で走らせる（networkd が遅くても login・
  logout を待たせない）。

### 確認（QEMU の証拠）

- build: lean（`BUILD=build/p1-net`、`plan/ws001/tests/config-amd64-lean-guest.mk`）・AX211 desktop（`build/p1-wdesk`）・試験 image
  （`build/p1-wdesk24`、[build-p024-image.sh](build-p024-image.sh)）・demo（`build/p1-desk2`）・Venus（`build/p1-venus`、[config-venus.mk](config-venus.mk)）で rc=0、
  変更した file の warning 0（`-Wall -Wextra -Werror`）。`git diff --check` は clean。
- **本物の networkd、radio 無し**（[session-check.sh](session-check.sh)、lean image、serial の対話、`build/p1-session.out`）: 起動後
  `owner=0 store=- sessions=-`。root で `open kei` → `sessions=1000`、`open root` → `1000,0`、`close root` → `1000`、`open 4242` → ENOENT、
  `close kei` → `-`。kei で `open`（自分）→ `1000`、`open root` → EPERM、`close guest` → EPERM。kei の `connect` は従来どおり `no WLAN radio`。
  kei の `close` → `-`。group 外の guest は `networkd is unavailable: Permission denied`。最後の logout の待ちだけが時間切れ（判定の後、影響無し）。
- **Venus の desktop（QEMU、radio 無し、本物の networkd と sessiond）**（[venus-session-check.sh](venus-session-check.sh)、`build/p1-venus-check.out`、
  画面 `build/p1-venus-shots/`）: 起動時の kei の自動 login の後 `sessions=1000` と sessiond.log の `SESSIOND NETWORK session open uid=1000`。
  App Home の Log Out の後 `sessions=-` と `session close uid=1000`（session の終わりの 2 秒後）、greeter（`m2-greeter.png` と同じ画面）。
  greeter で kei の password を入れて login → `sessions=1000` と 2 回目の `open`。port は P1 の範囲（[guest-ports.py](guest-ports.py) で 10100〜）。
- boot test: `plan/tools/boot-test.sh build/p1-desk2/hdd-image.img` → PASS（`build/p1-desk2-boot/login.png`、GPU の無い QEMU なので greeter が
  終わり console の login prompt）。

### 未実施・止めたこと

- **5330 の AX211 passthrough での自動接続（起動時の system の store、login 後の利用者の store、logout 後の切替え）は未実施**。試験 image
  （`build/p1-wdesk24`: autologin 無し、Wi-Fi の状態を `/var/log/p024-wifi.log` に記録する試験用 service）で 2 回起動し、2 回とも 5330 の host が
  QEMU の起動の約 1〜2 分後に network から消えた（1 回目 00:41、iGPU と AX211。2 回目 01:28、AX211 だけ・std VGA）。1 回目はユーザーが再起動し、
  host の journal は vfio の reset の 4 秒後で切れていた（Q1 の調べ、BUG-145 に経緯）。Q1 の指示（再現したら止めて返す）で止めた。原因は未解析
  （共通は試験 image。ただし journal の切れ方から QEMU 起動時の vfio の扱いの疑いも残る）。2 回目の後の /tmp/i915-hw.lock は解放していない
  （host の再起動の確認待ち、Q1 に連絡済み）。
- 2026-10-03 のユーザーの規則（iGPU と AX211 を同時に渡さない、WiFi の試験に iGPU を使わない）に合わせ、`wifi-desktop-qemu.sh` から iGPU を外し、
  `wifi-desktop-hw.sh` は iGPU の mode に触れないようにした（2 回目はこの形）。
- radio の上での自動接続そのものは、q611 の回帰（AX211 で lease が取れない、BUG-145）が直るまで成功の確認ができない見込み。
- networkd の再起動の後は、開いていた session が再び open されるまで利用者の store は候補にならない（次の login で戻る）。

### 再開の条件

5330 の host の hang の原因が分かり（または試験 image を使わない形で再現しないことが分かり）、BUG-145 の lease の回帰が直った後に、
AX211 だけの passthrough（std VGA の console）で: 起動直後（login 無し）の system の store からの試み、kei の store だけに AP を保存して
`net wifi session open`（sessiond と同じ要求）での試み、`close` での切断と system の store への切替え、を `p024-wifi.log` と `net wifi list` で確かめる。

### BUG-145 への追記（5330 の host の hang の手がかり、2026-10-03 P1）

| run | 時刻（JST） | image | QEMU に渡したもの | 結果 |
| --- | --- | --- | --- | --- |
| q611 a | 10-02 23:49 | `build/p1-wdesk`（p019 の版、kei の自動 login） | iGPU＋AX211、usb-net | 正常。終了後 AX211 は iwlwifi に戻った |
| q611 b（前の P1、i03） | 10-02 昼 | 同系統 | iGPU＋AX211 | 正常 |
| q612 1 | 10-03 00:41:44 | `build/p1-wdesk24`（p024 の networkd/sessiond、autologin 無し、試験用 service） | iGPU＋AX211 | 00:43 の最初の ctl から host に届かない。host の journal は 00:41:49（vfio の reset の 4 秒後）で途切れ、panic・pstore 無し（Q1 の調べ）。ユーザーが 00:55 に再起動 |
| q612 2 | 10-03 01:27:52 | `build/p1-wdesk24` | AX211 だけ（std VGA）、usb-net | 約 2 分後から host に届かない（ARP INCOMPLETE が続く）。未回復 |

- 2 回の hang の共通点は試験 image（p1-wdesk24）。違いは userland だけ: networkd の候補の読み方と session の要求、sessiond の通知、
  autologin 無し（greeter のまま、2 回目は GPU が無いので console の login）、`p024watch` service（root で `net watch` を常駐、2 秒ごとに
  `/home/kei/p024-system` を見る）。guest の software が host を止める経路は通常無いので、image の違いは偶然の可能性がある。
- journal が vfio の reset の 4 秒後で切れていることは、guest がまだ起動の早い段階（firmware〜kernel）の時点を指す。guest の networkd が
  動く前に host が止まった可能性が高い（1 回目）。2 回目の journal は未確認（host が止まったまま）。
- 切り分けの案（朝のユーザーの判断）: (a) 5330 を再起動し、`build/p1-wdesk`（q611 で正常だった image）を AX211 だけで起動して止まるかを見る。
  (b) 止まるなら host 側（AX211 の vfio の reset、10-02 の昼からの host の更新・設定の変化）を疑い、QEMU 無しで `vfio-pci` への bind/unbind
  だけを試す。(c) 止まらないなら p1-wdesk24 の違い（service と autologin）を一つずつ戻す。どの試験も host の journal を永続にしてから行う。

## q631-i01（P1 generation6、2026-10-03、base main `5ee632781`、commit `51b798274`・`be8822ec4`）

user の指示（2026-10-03）で p024 の自動再接続を RTL8822BU（開発機の TP-Link 2357:0138 の usb-host passthrough、local の QEMU）で試験する予定だった。
ws005-p020 の q631 の節の修正（拒否された鍵を候補から外す、明示の join の失敗の後に探索へ戻る、`net wifi add`・`modify`・`delete`、off の保持、
delete した接続中の network からの切断）は p024 の候補の読み方（`load_candidates`）と整合させてある（拒否は store ごと、PROFILES_CHANGED で解除）。
**p024 の通し（起動時の system の store、login の kei の store、logout の切替え）は未実施**（user の host 再起動のためのラップアップ）。
再開点と手順は [ws005-p020 の q631 の節](../phase020/phase.md) の「未実施（再開点）」の 5。5330 の AX211 の確認は user が最後に行う（Realtek で完全にした後）。

## clearance（2026-10-03 Q1、user の判断）

2026-10-03 user「あまりセキュリティ回避を行うとアカウントが削除される可能性があるので、いったんclearedにして先に進めましょう。あとで手作業で確認する作業としてWSを立てておいてください。」
- q631-i02（P1 generation7）: 全 image の build warning 0、boot-test PASS、store の host 試験の失敗は再現せず（修正なし）、kernel の SHUT_WR の POLLERR を [BUG-149](../../bugs/BUG-149.md) に記録。統合 01f4c5e05。
- q631-i03・i04（generation8・9）: 鍵の複写の片付けと `wifi-key.sh`（統合 880f625c1）。鍵の要る試験は権限の判定（Credential Materialization）で着手できず。guest に鍵は入っていない。
- 検証済みでない受け入れの項目（鍵の要る項目の全て、P3 の ws131-p003 の統合の後の libkeiland の join・forget の確認）は未実施のまま [WS133](../../ws133/ws.md) の手作業の確認へ移す。この clearance はそれらの PASS を主張しない。
