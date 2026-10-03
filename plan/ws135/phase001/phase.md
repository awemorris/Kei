<!-- awesome-plan project=zedbsd record=ws135-p001 -->
# ws135-p001: 設定の API と desktop.conf の設計

Status: planning
Disposition: normal
Parent: [WS135](../ws.md)
Queue: none

## 範囲

WS135 の「完了の条件」を満たす設計を `plan/ws135/design.md` に書く。

1. 今の設定の項目と読み手・書き手の棚卸し（`libkeiland/preferences.c`、`wayland/preferences.c`・`volume.c`、`settings/look.c`・`sound.c`、Terminal などの app 固有の設定、試験と道具）。
2. 項目ごとの解決の先（libkeiland が直接: 例 音量 → audiod / compositor の拡張: 壁紙・透明度・pointer・keyboard ほか）と、値の型・範囲・既定。
3. libkeiland の API: 読み・変更・監視（通知の callback、event loop への組み込み、別の process の変更の伝わり方）。WS131 の `kl_` の命名と backend の境界に合わせる。
4. compositor: 設定をメモリに持つ store、拡張の settings の interface（`kl_system_manager_v1` の version の予約）、session の開始の読みと終了の書き（Log Out・Shut Down・Restart・SIGTERM）、書きを event loop で待たない方法、監視の thread の除去。
5. 移行の順と Phase の分け方、試験（host・QEMU）、WS131 p010・p011 の表の書き換えの案。

## 受け入れ

design.md と Phase の表があり、design-reviewer の review を反映している。ユーザーに方針の要点を示す。
