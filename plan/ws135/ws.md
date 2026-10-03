<!-- awesome-plan project=zedbsd record=ws135 -->

# WS135: 設定の読み書きを libkeiland に一本化する（desktop.conf を compositor の内部に）

<!-- awesome-plan-current:start -->
Status: planning
Primary Milestone: MG006
Related Milestones: —
Objectives: O2
Parent: [Master](../master.md)
Queue: none
Resume point: [p001](phase001/phase.md)（設計）。WS131 の p010・p011（設定の部分）との分担は下の「WS131 との関係」。担当と開始の時期は Q1 がユーザーに確かめる。
<!-- awesome-plan-current:end -->

## 目標（2026-10-03 ユーザー、BUG-162）

「BUG-162ですが、WSを作ってください。設定の読み取りと変更はすべて、libkeilandを通して行います。libkeilandが直接解決する設定もあれば、コンポジタと拡張で通信して解決する設定もあります。設定項目を監視して変更の通知を受け取るインタフェースも必要です。desktop.confはコンポジタ内部の設定ファイルにして、ほかのプロセスからの利用はやめます。」

同日の先の指示:「desktop.confに書き込むのはコンポジタの都合であって、それはログインセッションの終了時に書き込んで再開時にロードするようにします。それ以外に書き込む必要はないです。」「デスクトップから変更する音量は。audiodに依頼をするのであって、あとのことはaudiodに任せてください。」

## 完了の条件

1. app（Settings・zdesktop の system bar・他の app）の設定の読み・変更・監視は、すべて libkeiland の API を通る。app は設定の file を開かない。
2. libkeiland の API は設定の項目ごとに解決の先を持つ: libkeiland が直接解決する項目（例: 音量は audiod へ）と、compositor と拡張の protocol で通信して解決する項目（壁紙・窓の透明度・pointer・keyboard の repeat など）。app からは区別が見えない。
3. 設定の項目を監視し、変更の通知を受け取る API がある（別の process での変更も届く。毎秒の file の poll ではない）。
4. `desktop.conf` は compositor の内部の file: session の開始で一度読み、終了（Log Out・Shut Down・Restart・compositor の終了）で一度書く。それ以外に読み書きしない。他の process は読まない・書かない。compositor の 1 秒ごとの監視の thread（`wayland/preferences.c`）を除く。
5. Settings の各頁と system bar が新しい API で動き、変更が即座（frame の単位）に効き、session をまたいで残る。旧 `keiland_preferences_*` の公開 API を除く。
6. zedBSD・Linux・FreeBSD の build、host 試験、QEMU の試験（T1/T2）、全文の規約の確認。

## WS131 との関係（Q1 の整理、2026-10-03）

WS131 の設計（`plan/ws131/design.md` §4.2〜4.4）は p010 で compositor の settings-store と拡張の `set`・`unset`、p011 で Settings の拡張への移行と監視の除去を予定していた。WS135 はこの設定の部分を引き取り、ユーザーの方針（読み書きは session の開始と終了だけ、監視・通知の API、libkeiland が解決先を選ぶ）で設計し直す。WS131 は拡張の manager の枠（`kl_system_manager_v1`）と network・audio・power を持ち、WS135 は manager に settings の interface を足す。WS131 の p010・p011 の表と design の settings の行の書き換えは p001 の結果で Q1 が行う。

## Phase

| Phase | 内容 | 状態 | 依存 |
| --- | --- | --- | --- |
| [p001](phase001/phase.md) | 設計: 設定の項目の一覧と解決の先、libkeiland の API（get・set・watch と通知）、compositor の拡張の interface（manager の version）、desktop.conf の session の開始・終了の読み書き、Settings・system bar・試験の移行、WS131 p010・p011 との分担、Phase の分け方。design-reviewer の review | planning | — |
| p002 以降 | p001 で決める（compositor の store と拡張、libkeiland の API と監視、Settings・system bar の移行と監視の thread の除去、Linux・FreeBSD、全文の規約と回帰） | planning | p001、WS131 の p010（manager の枠） |

## 関係

- [BUG-162](../bugs/BUG-162.md)（このWS の由来）、[BUG-161](../bugs/BUG-161.md)（音量は session の終わりに一度だけ書く、resolved）、BUG-125（毎秒の監視による menu の遅れ）。
- [WS131](../ws131/ws.md)（libkeiland と backend、拡張の manager）、WS089（Settings）、WS100（音量）、WS113（`displays.conf` も compositor の store）。
