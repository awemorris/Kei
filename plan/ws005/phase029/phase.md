<!-- awesome-plan project=zedbsd record=ws005-p029 -->
# ws005-p029: Wi-Fi の鍵の入力欄を選んだ AP の行の直下に出す（BUG-160）

Status: cleared（Q1 判定 2026-10-03: T1-019（QEMU）inline-key-bug160 PASS、鍵の欄が Neighbor 5G の行の直下。回帰 settings-p003・connecting-bug154 PASS、zdesktop-p013 は新しい guest で PASS。実機は S2）。元の記載: in-progress（実装済み・T1 の試験待ち）
Disposition: normal
Parent: [WS005](../ws.md)
Bug: [BUG-160](../../bugs/BUG-160.md)
Queue: q642（P1 generation11、2026-10-03。user「WiFiパスワードが画面の下の方になってしまうので、APの項目でインラインで入力できるようにP1でバグとして修正してください。」）

## 確かめたこと（読み）

- **system bar の network の menu（`userland/desktop/wayland/network.c`）が該当。** 鍵の欄（「Key for X」・欄・「Enter: join   Esc: cancel」）は
  network の一覧の全部の後（separator の下）に足されていた。鍵の短すぎ等の失敗の行は menu の一番下（有線の行・Disconnect の後）。
- Settings の Wi-Fi の頁（`page-network.c`）は既に AP の行の直下に欄と Show・Join・Cancel を出していた。ただし頁を scroll しないので、下の方の
  AP を選ぶと欄が pane の外（画面の下）に出て見えなかった。

## 実装

- `network.c`: 鍵の欄の 3 行（と失敗の行）を、選んだ AP の行の直後に入れる（`network_add_key_rows`）。scan に無くなった AP や Wi-Fi off の間は
  一覧の下に出す（従来の位置）。鍵の欄が開いている間は失敗の行を欄の下に出し、menu の下には出さない。行の上限を +12 に。
- `page-network.c`・`settings.h`: 欄が開いた次の frame で、AP の行と欄が pane に入るように頁を scroll する（`network_key_reveal`、一度だけ）。log
  `NETWORK key-form reveal scroll=N`。

## 検証

- build: `make -j8 ZEDBSD_CONFIG=plan/ws089/tests/config-amd64-settings.mk BUILD=build/p1-q638 build/p1-q638/bin/wayland` → exit 0、warning 0。Settings は
  host の `host-build.sh`（-Werror）で built。
- host: `build/ws089-host/settings-render --page=wifi --size=1180x520 control=103 draw=… hits` → `key-form reveal scroll=19`、欄（index 9）と Join・Cancel が
  pane の中（y=460〜496、pane の下端 508）。画面を見た（Neighbor 5G の直下に欄、Show・Join・Cancel）。`host-slot.sh` PASS。
- QEMU（T1 に依頼）: `plan/ws005/tests/inline-key-bug160.sh`（新規。menu の行の index が Neighbor 5G → Key for → 欄 → Enter の順で有線の行より前、
  短い鍵の失敗が欄の直後、PNG）と回帰の `plan/ws035/tests/zdesktop-p013.sh`・`plan/ws089/tests/settings-p003.sh`。未実施（結果待ち）。
- 実機は未実施。
