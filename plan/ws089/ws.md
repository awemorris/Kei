<!-- awesome-plan project=zedbsd record=ws089 -->

# WS089: 設定のアプリ（Settings）

<!-- awesome-plan-current:start -->
Status: incomplete
Primary Milestone: MG006
Related Milestones: MG002
Objectives: O2
Parent: [Master](../master.md)
Queue: なし
Resume point: **2026-09-29 ユーザーの指示でブラッシュアップは後回し**（「Settingsはある程度動いたらブラッシュアップは後回しにします。」main 経由）。p001〜p009 は cleared（p006 で規約の照合・回帰（8 つの guest の試験が PASS）・デモの通しを終えた）。新しい Phase は始めない。残りの候補は下の「後回しの候補」。WS の完了の処理（受け入れの確認、試験の plan/tools への移し、Phase の directory の削除）は main の判断。完了の後、Settings の libkeiui への移行（WS090 の p007）が始められる。試験の手順は各 phase.md と `plan/ws089/tests/`（`settings-regress.sh` が guest の試験の全部）。壁紙の生成は `userland/desktop/wallpapers/generate.py`
<!-- awesome-plan-current:end -->
作業の手引き（2026-10-01）: [guide.md](guide.md)

## 目標（2026-09-29 ユーザー）

「これもOSCデモで使う優先事項にしたいですが、設定画面のアプリを作ってほしいです。添付がイメージです。あくまでもイメージなので、この通りでなくていいです。
左側に項目のペイン、右側に設定項目。フローティングでセパレート。」

- 見本: [mockup-network.webp](mockup-network.webp)（Network の頁: 左に項目の一覧（Wi-Fi・Ethernet・Bluetooth・VPN・Network・Appearance・Wallpaper・
  Notifications・Sound・Display・Storage・Battery・Keyboard・Mouse・Touchpad・Printers・Sharing・Users・Privacy・Security・Accessibility・
  Updates・About）、上に戻る・進む・Home・breadcrumb・検索・表示の切り替え、右に card の群（接続の状態、Wi-Fi の一覧、Ethernet、VPN、
  DNS と Proxy、通信量の graph、初期化の button））。見本の通りでなくてよい。
- 形: 左の項目の pane と右の設定の pane を、それぞれ浮いた（floating）すりガラスの pane として分ける（Files（WS071）の付箋の pane と同じ作り）。
  窓は Keiland の浮いたタイトルバー（System Menu・CONTROLS）。名前の規則: 画面の文字は「Kei」、Keiland・libkeiland は出さない。
- デモの受け入れ（案、p001 で確定）: 起動して項目を選ぶと頁が替わり、少なくとも About（OS の名前・版・機械）、Network（networkd の実際の状態:
  interface・address・Wi-Fi の一覧と接続）、Display（解像度・拡大）、Appearance・Wallpaper（見た目と壁紙の変更が desktop に反映）、Sound（音量、
  audiod）、Mouse・Touchpad（速さ、慣性の scroll の on/off）が実際に働く。それ以外の項目は頁の枠と「準備中」の表示でよい。App Home から起動できる。
- **受け入れ（2026-09-29 確定）**: ユーザーの回答「設定項目は、ネットワークを中心にしてください。ディスプレイはまだスタブでいいです。」と
  main の判断による。
  1. 起動して項目を選ぶと頁が替わる（左の項目の pane と右の頁の pane、戻る・進む・breadcrumb）。App Home から起動できる。
  2. **Network が中心**: 接続の状態、Wi-Fi の一覧と接続・切断（**新しい network への鍵の入力を含む**）、Wi-Fi の入り切り、Ethernet、
     address・netmask・MAC・DNS、通信量（見本の Network の頁に近い内容）。networkd の protocol に最小の追加が要るなら
     [proposed/](proposed/) に案を置いてから実装する。
  3. About（OS の名前・版・機械）。
  4. Display は stub（読むだけの情報、または準備中）。accent の色と Touchpad は出さない（準備中）。
  5. Appearance・Wallpaper・Sound・Mouse・Keyboard は Network の後の優先度（p004・p005・p007）。デモの受け入れの必須は 1〜4。
  6. その他の項目は頁の枠と「準備中」。

## Phase

設計は [design.md](design.md)。他の WS の source への変更の案は [proposed/](proposed/)（main の許可が要る）。

| Phase | 目的 | Status | 依存 |
| --- | --- | --- | --- |
| [ws089-p001](phase001/phase.md) | 設計: 項目の一覧とデモで働かせる範囲、画面の構成（Files の pane・card の部品の再利用）、各項目の backend（networkd・audiod・sessiond・/dev/system・compositor の設定）との接続の方法、設定の保存先 | cleared（2026-09-29） | — |
| [ws089-p002](phase002/phase.md) | アプリの骨格: 窓、present、glass の 2 枚の card、titlebar（Back・Forward・Home・Breadcrumb・Sidebar）と履歴、System Menu、左の項目の pane、右の頁の scroll、Home（tile）・About・準備中の頁、App Home の行 | cleared（2026-09-29、Venus の guest） | p001、link の規則（main の許可済み） |
| [ws089-p008](phase008/phase.md) | 検索（titlebar の欄と結果の頁）と Home の tile の今の状態。App Home の歯車の絵（許可済み D4）も同時に | cleared（2026-09-29、Venus の guest。実機は未実施） | p002 |
| [ws089-p007](phase007/phase.md) | desktop の設定の仕組み: libkeiland の `keiland_preferences_*`（KEILAND_VERSION 13）と zdesktop の反映（[案](proposed/desktop-preferences.md)、許可済み D4） | cleared（2026-09-29、Venus の guest。実機は未実施） | p001、main の許可 |
| [ws089-p003](phase003/phase.md) | Network・Wi-Fi・Ethernet の頁（networkd の状態、Wi-Fi の一覧・接続・切断・新しい network の鍵、address・DNS・通信量。[案](proposed/libkeiland-network-link.md)、networkd の protocol は変えない） | cleared（2026-09-29、Venus の guest。実機は未実施） | p002 |
| [ws089-p004](phase004/phase.md) | Appearance・Wallpaper・Display・Storage の頁 | cleared（2026-09-29、Venus の guest。実機は未実施） | p002, p007 |
| [ws089-p005](phase005/phase.md) | Mouse・Keyboard の頁と Sound（audiod の有無の表示だけ、main の依頼）。Touchpad は準備中のまま | cleared（2026-09-29、Venus の guest。実機は未実施） | p002, p007 |
| [ws089-p009](phase009/phase.md) | 生成の壁紙（5 枚、1920x1080、`userland/desktop/wallpapers/generate.py`）を build の時に作り、デモの image に同梱（2026-09-29 ユーザーの D8 の判断、main の許可） | cleared（2026-09-29、Venus の guest。デモの image は `make -n`。実機は未実施） | p004 |
| [ws089-p006](phase006/phase.md) | 規約の全文との照合、回帰、デモの通し（App Home の絵は p008、デモの image の壁紙は p009） | cleared（2026-09-29、Venus の guest。実機は未実施） | p003〜p005, p008 |

## ユーザーの判断（2026-09-29 に master から移した）

| 項目 | 決定 | 記録先 |
| --- | --- | --- |
| 設定のアプリ（2026-09-29） | ユーザー:「これもOSCデモで使う優先事項にしたいですが、設定画面のアプリを作ってほしいです。添付がイメージです。あくまでもイメージなので、この通りでなくていいです。左側に項目のペイン、右側に設定項目。フローティングでセパレート。」 | [WS089](ws089/ws.md) |
| 設定のアプリの範囲（2026-09-29 夜） | ws089-p001 の問い（Display の拡大は表示だけ・accent の色は出さない・Touchpad は準備中）にユーザー:「設定項目は、ネットワークを中心にしてください。ディスプレイはまだスタブでいいです。」→ Network（Wi-Fi・Ethernet・状態・新しい Wi-Fi への鍵の入力を含む）を中心に作り込む。Display はスタブ、accent と Touchpad は出さない（準備中） | WS089 |

## 後回しの候補（2026-09-29 ユーザーの指示「Settingsはある程度動いたらブラッシュアップは後回しにします。」）

新しい WS か、この WS の再開で扱う。どれも未着手。

| 候補 | 内容 | 要るもの |
| --- | --- | --- |
| Touchpad | 頁（速さ・tap・慣性の scroll） | kernel の touchpad の driver（D9） |
| 音量 | Sound の頁の出力の音量・mute（今は audiod の有無の表示だけ） | [proposed/libkeiland-audio.md](proposed/libkeiland-audio.md)（libkeiland の追加）、HDA の driver、デモの image の audiod |
| Display の変更 | 解像度・拡大の変更（今は読むだけ） | zdesktop の出力の scale（D1） |
| accent の色・dark | Appearance の色の選択 | zdesktop と各 app の固定の色を preferences から読む（D2） |
| 暗い壁紙の上の文字 | Aurora・Twilight の上ですりガラスの上の文字の contrast が下がる | zdesktop の glass の tint（main が WS035 に回した） |
| About の memory | memory の行 | [proposed/libkeiland-system.md](proposed/libkeiland-system.md) |
| 準備中の頁 | Bluetooth・VPN・Notifications・Battery・Printers・Sharing・Users・Privacy・Security・Accessibility・Updates | 各機能の backend |
| 通信量の履歴 | 24 時間の graph（今は窓を開いてから） | daemon の記録 |
| 検索の key 操作 | 結果の上下の選択 | settings の中だけ |
| touch の drag の scroll | 頁の pane の drag（`keiland_scroller`） | settings の中だけ |
| 単一の instance | 二つ目の起動で既存の窓を前に | settings の中だけ |
| 日本語の UI | files の F-041 と一緒に | text の翻訳の仕組み |

## WS の完了の処理に要ること（main の判断）

- 受け入れ（上の「受け入れ（2026-09-29 確定）」1〜6）は QEMU で満たした。実機の確認は未実施（main の判断で実機の確認に回す）。
- 試験の移し先の候補（`plan/tools/settings/` として Tools 節に登録）: `settings-regress.sh`・`settings-wait.sh`・`settings-guest.sh`・
  `build-settings-image.sh`・`config-amd64-settings.mk`・`settings-p002.sh`〜`p009.sh`（名前を役割の名前に変える）・`host-build.sh`・
  `host-render.c`・`host-network.c`・`host-preferences.c`・`host-preferences.sh`。network-probe（WS035 の試験 program）の stand-in の振る舞いは
  `settings-p003.sh` が使う。
- proposed の整理: desktop-preferences・libkeiland-network-link・vmunix-link・app-home-icon は適用済み、libkeiland-audio・libkeiland-system は
  未適用（後回しの候補）。
- 完了の後、Settings を libkeiui に移す作業（WS090 の p007）が始められる。settings は files の canvas・text・icons を source で共有して
  いる（`userland/desktop/files/canvas.c`・`text.c`・`icons.c` と `artwork/mark.c`、Makefile と host-build.sh）。

## 後続のDisplay機能 / 2026-10-02

Event ws113-multidisplay-plan-20261002-ws089-followup: userは読み取り専用のDisplay stubを、外部display/全拡張・全mirror/drag配置ができる[WS113](../ws113/ws.md)で後日実装するよう指定。Settingsはlibkeilandの公開APIだけからcompositor拡張へ接続する。WS089の過去のNetwork中心/stub受け入れとp001〜p009の結果はそのまま保持。新WSはplanned、WS089の未実行Phase/Queueを自動開始しない。GitHub comment/body反映保留。
