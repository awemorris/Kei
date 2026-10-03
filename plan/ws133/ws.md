<!-- awesome-plan project=zedbsd record=ws133 -->

# WS133: 安定版 S1 の実機試験

<!-- awesome-plan-current:start -->
Status: incomplete
Primary Milestone: MG003
Related Milestones: MG005、MG006
Objectives: O1, O2
Parent: [Master](../master.md)
Queue: none
Resume point: 16:30 に main を締め、17:00 までに S1 の image と手順書、17:00 に 5330 で実機試験（2026-10-03）
<!-- awesome-plan-current:end -->

## 進め方（2026-10-03 ユーザー）

2026-10-03 user「えーと、WiFi試験は、実機でやります。zedBSDの安定版を作って、実機で起動し、SSHで接続して試験を行います。1つの安定版に、複数の実機試験を詰め込みます。WiFiに限らずです。さまざまな試験をそこで行い、結果をいくつかのWSで修正して、また別な安定版を作り、そこで実機試験を詰め込みます。」

安定版 S1 の image を作り、実機で起動して SSH で接続し、下の試験をまとめて行う。結果の不具合は各 WS で直し、次の安定版（S2、別の WS）でまた試験を詰め込む。

## 由来（WiFi の項目）

2026-10-03 user「あまりセキュリティ回避を行うとアカウントが削除される可能性があるので、いったんclearedにして先に進めましょう。あとで手作業で確認する作業としてWSを立てておいてください。」
[ws005-p020](../ws005/phase020/phase.md)・[ws005-p024](../ws005/phase024/phase.md) は、鍵の要る試験がエージェントの権限の判定（Credential Materialization）で行えなかったため、ユーザーの判断で cleared にした。その未実施の項目をここで手作業で確かめる。エージェントは鍵を扱わない。手順書と image の用意、結果の記録はエージェントが行える。

## 試験の項目

### WiFi（ws005-p020・p024 から移管）

実機（安定版 S1）で、SSH で接続して確かめる。鍵の入力の方法（ユーザーが実機の console で `net wifi add` するなど、エージェントが鍵を扱わない形）は S1 の準備で決める:

1. `net wifi add`（root・kei、2.4GHz・5GHz）、各回の lease・ping・`net wifi list` の `ssid=`。
2. 誤った鍵 → 拒否（"Wi-Fi key refused"）→ `modify` で正しい鍵 → 接続。`--auto no` → 接続しない → `modify --auto yes` → 接続。見えない SSID の add、接続中の add（今の接続を保つ）、連続の add・modify。
3. Settings からの AP の切替え（2.4↔5GHz）、誤った鍵で別の AP へ join → もとの AP に戻る。
4. guest（機械）の再起動の後の再接続、off の保持（off で再起動 → off、on で再起動 → on）、接続中の network の delete → 切断と探索。
5. p024: 起動時の system の store での自動再接続、login で kei の store、logout で除去。
6. [ws131-p003](../ws131/phase003/phase.md) の統合の後: Settings・system bar からの誤った鍵・正しい鍵の join（BUG-149 の回避の確認）、拒否された鍵の削除（ユーザーの決定 (2)）。

手順の土台（QEMU の passthrough 用、実機に合わせて直す）: `plan/ws005/phase020/rtl-guest.sh`・`build-rtl-image.sh`・`wifi-key.sh`、[phase020](../ws005/phase020/phase.md) の q631-i01 の節の「未実施（再開点）」。

## 受け入れ

各項目の PASS/FAIL をユーザーの観察（画面・`net wifi list`）で記録する。QEMU の passthrough か実機かを分けて書く。FAIL は Bug にして WS005 へ。

## Phase

| Phase | 内容 | 状態 | 依存 |
| --- | --- | --- | --- |
| p001 | 安定版 S1 の内容（base の commit・対象の機種）と試験の一覧を決め、image と手順書を作る | planning | ユーザーの決定 |
| p002 | 実機で S1 を起動し、SSH で試験を流して結果を記録（WiFi の 1〜6 を含む） | planning | p001、ws131-p003 の統合（WiFi の 6） |

### 他の WS の実機の候補（2026-10-03 Q1 が Master から拾った候補、採否はユーザー）

- [WS099](../ws099/ws.md) p012: 5330 の C1 の目視、BUG-119（電源断）・BUG-122
- [WS094](../ws094/ws.md) p012: desktop の icon の 5330 の性能・操作
- [WS100](../ws100/ws.md) A7: 5330 の speaker・headphone
- [WS005](../ws005/ws.md) p023: B5（5330 の AX211・USB の LAN、5320 の USB の LAN・無線）
- [WS033](../ws033/ws.md): USB の LAN の抜き差し
- [BUG-134](../bugs/BUG-134.md): AX211 の実機試験の image（ws004-p051）
- [BUG-105](../bugs/BUG-105.md): USB マウス（Logi Bolt）
- [WS118](../ws118/ws.md): 5320 の LCD（P4 の q634 の続き、agent/p4 59a94c6aa は未統合）
- [WS079](../ws079/ws.md): Notes・PDF Viewer の 5330 の mouse

## S1（2026-10-03 13:30 Q1 の案、user 承認「S1案、了解です。16:30に締めて、17時までにイメージを用意してください。17時に実機テストします。」。元の依頼: ユーザーの「S1は本日夕方に実施するので、それまでに修正してテストできそうな内容を選んでおいてください。」による。ユーザーの確認待ち）

時刻の案: 16:30 に main を締める（P1 の BUG-149・P3 の WS131 p003 が統合できていれば含める、間に合わなければ外す）→ S1 の image の build・`boot-test.sh`・手順書 → 夕方に 5330 の素の起動（USB）で SSH の試験とユーザーの目視。

| 群 | 項目 | 出元 | 状態 |
| --- | --- | --- | --- |
| 確認だけ（修正済み・統合済み） | WiFi の add・modify・delete・誤った鍵・`--auto`・off の保持・再起動後の再接続・AP の切替（鍵はユーザーが console で入れる） | WS005 p019・p020・p024 | main に統合済み |
| 同 | C1 の切り替え（黒 0・文字 0）、Shut Down（BUG-119・BUG-122） | WS099 p012 | 統合済み |
| 同 | desktop の icon の操作と速さ | WS094 p012 | 統合済み |
| 同 | Files の主な操作、BUG-141・142 の再確認 | WS127 p007 | 統合済み |
| 同 | 標準アプリの通し、Notes・PDF Viewer の S8・S9、CJK Ambiguous Width | WS128 p007・p009、WS079 | 統合済み |
| 同 | Settings の S7・透明度 | WS089 p011 | 統合済み |
| 同 | IME（A／あ、変換中の文字、候補の窓） | WS095 | 統合済み |
| 同 | Terminal のタイトルバー、Gears | BUG-137・136 | 統合済み |
| 同 | USB マウス（Logi Bolt） | BUG-105 | 統合済み |
| 同 | 素の UEFI の起動の画面の引き継ぎ（この起動そのもの） | WS084 | 統合済み |
| 同 | speaker・headphone（任意） | WS100 A7 | 統合済み |
| 観察 | AX211 の DHCP・5GHz（passthrough でなく素の起動） | BUG-145・BUG-134 | 修正は未。素の起動で症状が出るかを見る |
| 観察 | USB の LAN（RTL8156）の DHCP・抜き差し | WS033 p002、WS005 p023 | ws033-p001（QEMU）は未完。素の起動での挙動を見る |
| 観察 | Terminal で Emacs の描画の開始の行 | BUG-150 | zedBSD でも起きるかを見る |
| 今日の修正（間に合えば） | Settings・system bar からの join（EIO が出ない） | BUG-149（P1 ws005-p025）、WS131 p003（P3） | 作業中 |

外す: 5320（FreeBSD が入っていて P3 が使用中、WS118 の P4 の branch は未統合）、未実装の WS、別の機種・外付けの機器の要る項目。
