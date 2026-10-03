<!-- awesome-plan project=zedbsd record=ws005-p030 -->
# ws005-p030: system bar の network の menu — 接続中の AP を一番上に Disconnect の button、画面に収まる件数（BUG-148）

Status: in-progress（実装済み・T1 の試験待ち）
Disposition: normal
Parent: [WS005](../ws.md)
Bug: [BUG-148](../../bugs/BUG-148.md)
Queue: q643（P1 generation11、2026-10-03。user「優先のバグを解決し、そのあと実機なしで解決できるバグをどんどん処理してください。」）

## 範囲

AP が多いと menu の下端の「Disconnect from X」が画面の外に切れた。user の希望: 接続中の AP を一番上に出し、その行に Disconnect の button。

## 実装（`userland/desktop/wayland/network.c`）

- 接続中（CONNECTED）の AP の行を一覧の最初に置く（scan に無ければ専用の行 `NETWORK_ROW_CURRENT`）。行には check・SSID・右端に「Disconnect」の
  button（幅 92、pointer の下で青）。button の click で disconnect、行の他の所は何もしない（行の青い強調もしない）。下端の「Disconnect from X」の行は無くした。
- menu が画面（`server->height - menu_y - 8`）より高い時は、他の AP の行を後ろ（弱い方）から減らし、「N more in Settings > Wi-Fi」の note を出す。
  接続中の AP と鍵を入力中の AP は必ず出す。
- log: `ZWL NETWORK disconnect x= y= width= height= ssid=`（試験が click する所）。layout の checksum に Wi-Fi の状態を含める（button の出入りで再 log）。
- 試験の直し: `plan/ws035/tests/zdesktop-p013.sh` の「Disconnect from X」の行の期待を button の log に。新しい試験 `plan/ws005/tests/menu-bug148.sh`。

## 検証

- build: `make … BUILD=build/p1-q638 build/p1-q638/bin/wayland` → exit 0、warning 0。
- host 試験: 無し（compositor の描画）。読みで確かめた。
- QEMU（T1 に依頼）: `menu-bug148.sh`（1280x800 で Kei Lab に join して最初の行・button の log・「Disconnect from」の行が無い、1280x230 で
  「2 more in Settings > Wi-Fi」と高さ 182 以内、button の click で op=36）、回帰の `zdesktop-p013.sh`・`connecting-bug154.sh`・`inline-key-bug160.sh`。未実施（結果待ち）。
