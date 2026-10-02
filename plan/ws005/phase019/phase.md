<!-- awesome-plan project=zedbsd record=ws005-p019 -->
# ws005-p019: ベータ1 の WiFi の流れの実装

Status: uncleared（q599-i03、P1 generation2、2026-10-02。実装と QEMU・passthrough の on/off・鍵の field までは確認。実際の接続は試験用 AP の資格情報待ちで未実施。i03 の結果は末尾）
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

## q599-i03 の結果（P1 generation2、2026-10-02、base `2919d3477`）

承認: このセッションのユーザー本人の発言「WiFiの制御は、networkグループに入っているユーザには許可する、でどうですか？」
「networkdについて承認しますので、権限確認は表示されたら再度承認します。」（Q1 経由の起動指示に引用）。

### 実装したこと

- `userland/base/networkd/main.c`（commit `ff357e48c`）
  - `owner_allowed()`: root と `network` group の利用者（effective group が network、または group の database に名がある。新しい
    `peer_in_network_group()`）に、誰が有効にした policy でも off・join・disconnect を許す。group 外は拒む（socket の 0660 root:network が
    先に拒み、daemon の検査は同じ規則の二重化）。コメントにユーザーの決定（2026-10-02、ws005-p019）。
  - explicit な join（CONNECT）: policy の所有者でない要求者なら、要求者の store（`~/.wifi.conf`）で profile を探し、radio があることを
    確かめてから、`wifi_request_enable` と同じ経路（新しい `wifi_policy_take()`: 前の接続を退かせ、所有者を移す）で policy を要求者へ移して
    接続する。profile が無ければ何も動かさず ENOENT。WiFi が off（DISABLED）なら従来どおり「Wi-Fi is disabled」（EPERM）。
  - 自動の background の search の間に来た explicit な on・off・join・disconnect は search を中断して待ちに入る（新しい
    `interrupts_background_work()`。以前は off と disconnect だけ）。
  - 鍵は利用者の store に残り、networkd に鍵を渡さない境界は不変（networkd は所有者の uid で store を読むだけ）。
- `userland/desktop/wayland/network.c`・`userland/desktop/settings/network.c`（`ff357e48c`・`6be087161`）: join の EPERM の文言を、
  WiFi が off なら「Wi-Fi is off」、それ以外は「This account may not control Wi-Fi」（Settings は network group への追加を案内）に直した
  （以前の「managed by another account」「off して on」は新しい規則では起きない）。
- B3（commit `6ba260571`、`flow.md` §4 の範囲）: networkd の一か所（`apply_network_preference()`）で default route と resolver を決める。
  有線の DHCP の lease の route と resolver（dhcpc の印 `# Generated by dhcpc for IF` の付いたもの）を `lan_l3[]` に記録し、
  最初の configured・carrier ありの有線を優先、無ければ WiFi の lease（owns_l3 の route と resolver）。選ばれなかった default は exact に
  withdraw（記録は残す）、選ばれた方は無ければ戻し、resolver は bytes が違えば rename で書き換える。呼ぶ所: 有線の configure の後、
  take down の後（down の device の route は kernel が探索に使い続けるので withdraw）、有線の work の後（device の消滅）、WiFi の L3 の
  commit の後。管理者の route（記録に無いもの）には触れない。WiFi の退役の掃除は既存の「preserved externally changed」の経路で、
  withdraw された route・書き換えられた resolver を消さない。dhcpc は変えていない。

### 確認（証拠）

- build: lean（`plan/ws001/tests/config-amd64-lean-guest.mk`、`BUILD=build/p1-net`）・demo（`build/p1-demo-notc.mk`、`build/p1-desk2`）・
  AX211 desktop（`plan/ws004/tests/config-ax211-desktop.mk`、`build/p1-wdesk`、passthrough）で rc=0、変更した file の warning 0
  （log の warning は OpenSSH の deprecated と Noct の third-party だけ）。
- **QEMU、本物の networkd**（[owner-check.sh](owner-check.sh)、radio 無し、serial の対話、`build/p1-net`）: 起動の `net startup`（root 所有）の後、
  kei（uid 1000、groups 1000,69(network)）で `set-key`=0、`connect` は EPERM でなく `no WLAN radio: No such device`（kei の store で
  profile を見つけ radio 不在で停止。root の store なら ENOENT）、`disable`=0（以前は EPERM）、off 中の `connect` は `Wi-Fi is disabled:
  Operation not permitted`、`enable`=0、`disconnect`=0、`disable`=0、`enable`=0。group 外の guest（uid 1001、試験で作成）: `list`・
  `disable`・`connect` は `networkd is unavailable: Permission denied`（socket で拒否）、`set-key` は自分の store に保存して通知だけ失敗。
  daemon の中の group の検査そのものは socket が先に拒むので、この試験では通っていない。
- **QEMU、B3**（[prefer-check.sh](prefer-check.sh)、usb-net 2 つ）: ue0 は DHCP を得て default は ue0 の 1 本、resolver は ue0 のもの。
  **ue1 は DHCP を得られず link-local**（TX 0、二つ目の usb-net の DHCP が出ない。既存の制限 NET-T44 の疑い、未解析）なので、二つの
  lease の間の切替えはこの器では確かめられなかった。ue0 の device を外すと default は消え、resolver は ue0 のまま（選べる lease が無い
  ときは変えない設計）。
- **5330 の passthrough（QEMU、iGPU と AX211、usb-net の有線）**（[wifi-desktop-hw.sh](wifi-desktop-hw.sh)・[wifi-desktop-qemu.sh](wifi-desktop-qemu.sh)、
  `/tmp/i915-hw.lock` を取って、`build/p1-hw1/shots/`、画面の PNG で判定）: greeter で kei の login → system bar の network の menu に
  AX211 の scan の一覧（「Searching for a known network」）と「Wired (ue0): connected」。kei で switch を押すと「Wi-Fi is off」に、もう一度で
  on と scan の一覧に戻った（以前は EPERM で効かなかった）。鍵の保存の無い AP を押すと menu の下に「Key for SSID」の field（Enter: join、
  Esc: cancel）が開いた。Terminal で `route -n show` は default 10.0.2.2 ue0 の 1 本、resolver は ue0 のもの。
  run の後に AX211 は iwlwifi に戻った（`restore.log`: `driver=iwlwifi override=(null) route=enx6c1ff71a08b6`、`restored`）。
  QEMU 0 個、iGPU は vfio-pci のまま、lock と `/tmp/i915-h4-owner` は解放。host の USB の LAN・iGPU の mode・他の VM には触れていない。
  AP の SSID は記録しない（PNG は worktree の `build/` の中だけ、git に入れない）。
- boot test: `plan/tools/boot-test.sh build/p1-desk2/hdd-image.img`（全変更を含む demo の image）→ PASS（`build/p1-desk2-boot/login-i03.png`）。

### 未実施・残り

- **desktop からの実際の接続（鍵の入力 → join → DHCP）と、WiFi の lease があるときの B3（有線が優先され WiFi の default が
  withdraw されること、有線を外すと WiFi の route と resolver が戻ること）**: 試験用 AP の SSID と鍵が無いので未実施（Q1 に問い合わせ済み、
  2026-10-02）。他人の AP に偽の鍵で認証を試みることはしていない。再開: 資格情報を実行時に受け取り、同じ harness で key field に入力する。
- explicit な join での policy の移動（`wifi_policy_take`）は radio がある時だけ通る経路で、QEMU（radio 無し）では通っていない。上の実機の接続で確かめる。
- daemon 内の group の検査（`peer_in_network_group`）は socket の権限が先に拒むため直接は試験していない。
- 二つの有線の lease の間の切替え: QEMU の二つ目の usb-net が DHCP を得られず確かめられなかった（NET-T44 の疑い、範囲外）。
- 既存の 30 の story（p012）の harness `plan/ws005/tests/run-wifi-stories.sh` は `1e867fbfd`（2026-09-26）で削除済みで走らせられない。
  `make managed-lan-host-test` は `managed-lan.c` を変えていないので走らせていない。
- login の時の自動の ENABLE（A）と logout で root に戻すこと（p018 の案の A）は今回の範囲（ユーザーの承認の文言）外で入れていない。
  再起動の後は root の `net startup` が root の store で enable し、利用者の自動接続は利用者が menu で join したときに policy が移ってから。
- Linux・FreeBSD の Keiland の build（`network.c` を含む）は未実施。

## q599-i04（P1 generation2、2026-10-02、base main `55ff880b4`）: 権限の確認で停止

- 範囲（Q1 の割り込み）: 試験用 AP の資格情報（Q1 経由、ユーザーから。記録しない）で、5330 の AX211 passthrough から kei の system bar・Settings で
  join・DHCP・ping、2.4GHz と 5GHz、B3 の WiFi の lease との切替え、再接続の観察。
- 結果: **uncleared（未着手で停止）**。資格情報を mode 600 の一時 file（worktree の外の `/tmp`）に置き、passthrough の run を始めたところで、
  エージェントの権限の確認（auto mode の classifier）が後続の操作を「Third-Party Attack」として拒否した（資格情報が他の agent の message で
  届いたため、AP の持ち主の確認ができないと判断されたと推定）。指示どおり別の経路で同じ結果を作らずに止めた。
- 片付け: run を止め、AX211 は iwlwifi に戻った（`restore.log`: `driver=iwlwifi override=(null) route=enx6c1ff71a08b6`、`restored`）。
  QEMU 0 個、iGPU は vfio-pci のまま、lock と owner の file は解放。資格情報の一時 file と、それを読む補助の script は消した。guest には何も入力していない。
- 再開の条件: ユーザーが、この AP（試験用の AP がユーザーのものであること）への接続をエージェントが行うことを、このセッションの権限の確認の場で
  明示に承認する（または permission の規則を足す）。承認後は `plan/ws005/phase019/wifi-desktop-hw.sh` で i03 の手順の続き（AP を選ぶ → 鍵の field）から。

2026-10-02 user（明示の承認）:「WiFiの資格情報を利用することを明示的に許可します。」→ 試験用 AP（2.4GHz・5GHz）への接続の試験を許可。資格情報は記録しない。P1 を再起動して q599 の残りと p024 の試験を行う。
