<!-- awesome-plan project=zedbsd record=ws005-p019 -->
# ws005-p019: ベータ1 の WiFi の流れの実装

Status: uncleared（q599-i01、P1、2026-10-02。system bar の鍵の入力と待ちの slot は実装。所有者の扱い（A＋A′）は止めてユーザーの判断待ち）
Disposition: normal
Parent: [WS005](../ws.md)
Focused goal: fg019（ベータ1）
Queue: none
目安: 3〜4h

## 2026-10-02 のユーザーの指示と採用した方式

ユーザー（5330 実機の後）:「画面右上のWiFiメニューについて、WiFiオンオフボタンがクリックしても実装されていませんでした。個別のWiFi APを選んでもパスワード入力や接続ができませんでした。実装をお願いします。優先度高いです。」（[BUG-138](../../bugs/BUG-138.md)）。
範囲に system bar の WiFi menu の on/off と、AP の選択 → 鍵の入力 → 接続を含める（p018 の判断 2 はユーザーの要求で「入れる」）。方式は p018 の推奨 **A＋A′**（login で利用者として enable、未所有なら join の前に enable、logout で root へ）、有線と WiFi は**有線を優先し route と DNS は networkd が一か所で決める**、複数の利用者の間では A′ の取り上げを許す（ベータ1）を既定として進める。ユーザーが異を唱えたら直す。実 radio の確認は AX211 の 5330 passthrough（BUG-134 解決済み）で行う。

## 範囲

p018 の flow.md で決めた流れを最小の変更で実装する。予定の対象（p018 で確定）: `userland/base/networkd/`（所有者と store の扱い）、`userland/base/net/`、
`userland/desktop/libkeiland/zedbsd/network-zedbsd.c`（要求の順）、login 時の enable を選んだときの `userland/desktop/sessiond/`。
Settings（WS089）・system bar（WS035）の画面の source を変える必要があれば、その差分は main に依頼する（このPhaseの所有の外）。
networkd の protocol（ZNV2）の変更は p002 の固定した契約の範囲で行い、外れるなら止めて main に返す。

## 受け入れ

- 本物の networkd と偽の `wifi` の子（既存の P012 の story の仕組み）で: 所有者が root の状態から利用者の鍵つき join が通る、再起動相当（networkd の再起動と
  login）の後に p018 で決めた方法で再接続する、鍵の誤り・AP の不在・off で理由が返り daemon が止まらない。
- 既存の 30 の story（p012）と WS033 の `make managed-lan-host-test` が通る。build の warning 0。新しい code は `plan/coding-style.md` の全文。
- 最後に `plan/tools/boot-test.sh` で login prompt（PNG をユーザーに見せる）。

## 検証

変更した領域の host の試験（WS005 の story、managed-lan、libkeiland の network の試験があればそれ）と boot test。集約の `make check` は走らせない。

## 所有 path

上の範囲の source、`plan/ws005/phase019/`、`plan/ws005/tests/`。

## 依存

p018、ユーザーの判断（p018 の未決の判断）。WS033 p001 と `userland/base/networkd/`・`userland/base/net/` が重なるので同時に走らせない。

## 未決の判断

p018 の結果による。

## q599-i01 の結果（P1、2026-10-02、base は main `a48948669` を merge した `4a835cb24`）

### 実装したこと（`userland/desktop/wayland/network.c` だけ）

- **鍵の入力（BUG-138 の「パスワード入力ができない」）**: menu の AP を押すと、鍵を要し（`secured`）利用者の store に保存の無い network なら menu の中に
  鍵の field（「Key for SSID」、`*` だけを表示、Backspace、Shift と Caps Lock、Enter で決定、Esc で field だけを閉じる）を開く。Enter で
  `keiland_network_save_key`（利用者の `~/.wifi.conf`）→ 鍵の buffer を volatile で消去 → `PROFILES` → その答えで `JOIN`（Settings と同じ 3 段）。
  8 文字未満は「The key must be 8 to 63 characters」。鍵の無い join の ENOENT でも、secured の AP なら field を開く。menu を閉じると入力中の鍵も消す。
- **待ちの slot（「on/off を押しても何も起きない」の一因）**: libkeiland は request を一つしか持てず、menu を開いたときの scan が出ている間に押した
  switch・AP は `EBUSY` で失敗の文になっていた。押した request を一つの slot に置き、出ている request の答えの後に送る（scan は置かない）。
- 失敗の文: join の EPERM は「Wi-Fi is managed by another account」、PROFILES の失敗は「Could not save the key (...)」。
- 確かめ: `plan/ws075/demo/build-demo-image.sh build/p1-desk2 ZEDBSD_CONFIG=build/p1-demo-notc.mk`（CI 土台のデモ config から clang・libcxx・remacs を除く
  一時の config）で `network.c` は `-Wall -Wextra -Werror` で warning 0、rc=0。`plan/tools/boot-test.sh build/p1-desk2/hdd-image.img` → PASS
  （`build/p1-desk2-boot/login.png`）。画面での鍵の field の操作と、本物の radio（5330 の AX211 passthrough）での接続は**未実施**。Linux・FreeBSD の
  Keiland の build（`Makefile.linux`・`Makefile.freebsd` も `network.c` を含む）は未実施（標準の C と既存の libkeiland の関数だけを使った）。

### 止めたこと（判断が要る）

- p018 で確かめたとおり、起動の `net startup`（root）が方針の所有者を root にし、利用者の join と off は networkd の `owner_allowed` で EPERM になる
  （on/off が効かない・接続できないの主因）。採用の方式 A＋A′ を networkd で実装しようとした変更（`owner_allowed` を network group の利用者に
  広げ、explicit な connect で所有者を移す）は、**エージェントの権限の確認（auto mode の classifier）に「Security Weaken」として拒否された**。
  同じ結果を別の経路（libkeiland・system bar・Settings から join の前に `ENABLE` を送って所有者を取る、login の時の自動の `ENABLE`）で達するのも
  同じ判断の対象とみなし、行っていない。networkd の source は変えていない（試した変更は戻した）。
- 必要な判断（ユーザー）: 次のどれかを、ユーザーが明示に承認すること。
  1. networkd: network group の利用者が root（または他の利用者）の WiFi の方針を操作し、join で所有者を自分に移せるようにする（A′ を server で）。
  2. client: system bar・Settings が、所有者でないときに既存の `ENABLE`（今も network group の利用者に許される操作）を先に送り所有者を取る（A′ を client で）、
     と login の時の自動の `ENABLE`（A）。
  3. 別の方式（例: 起動時の `net startup` が WiFi を有効にしない、または console の login の利用者を所有者にする）。
  承認が得られれば、承認の文言を引用して実装を再開する。それまで BUG-138 の「on/off」「接続」は、利用者が root でない限り EPERM のまま。
- B3（有線優先、route と DNS の一元化）は未着手。


## 2026-10-02 user（明示の承認）:「WiFiの制御は、networkグループに入っているユーザには許可する、でどうですか？」

networkd の owner の確認を広げ、`network` group の利用者には、root や他の利用者が持つ WiFi の policy の操作（on/off、join、key の通知の受理）を許可する（p018/p019 の選択肢 1）。permission system が止めた変更（`owner_allowed` の拡張、explicit な join での owner の移動、explicit な要求による自動の search の中断）はこの承認の範囲。`network` group に入っていない利用者は従来どおり EPERM。ベータ1 のデモの利用者（kei）が `network` group に入るかを確かめる。q599-i02 で再開。
