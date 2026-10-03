<!-- awesome-plan project=zedbsd record=ws005-p028 -->
# ws005-p028: BUG-157 — 失敗した接続の理由が、その後の close・quiesce で消えて ENETDOWN になる

Status: in-progress（実装済み・確認は S2 の実機）
Disposition: normal
Parent: [WS005](../ws.md)
Bug: [BUG-157](../../bugs/BUG-157.md)
Queue: q641（P1 generation11、2026-10-03。Q1 の dispatch。承認: user「実機がなくても修正できるバグをP1で修正、T1で順次テスト、のパイプラインを実行してください。」「実行してください。」）

## 原因（読みと host 試験）

5330 の AX211 で間違えた鍵が「Could not join (Network is down)」（ENETDOWN）になった。QEMU の RTL8822BU では EACCES。

- 鍵の拒否は共通の `src/kern/net/wifi/wlan.c` が分類する（M2 の後の deauth → `station_link_lost_controlled` が state FAILED・terminal_error EACCES）。
  AX211 もこの経路を通る。
- **共通の `station_retire_controlled` は、station の close・driver の quiesce（AX211 の `ax211_pci_close_locked` の `wlan_station_close`・
  `wlan_station_quiesce_begin`、detach、shutdown）で terminal_error を 0 にし、state を DOWN にしていた。** 接続の失敗の後に AX211 が recovery・
  session stop・close に入ると（RTL8822BU には無い経路）、その generation の status は「DOWN、理由なし」になる。wifi の tool（machine mode）の
  `wait_for_connection` は terminal_error が 0 で state が DOWN の時に **ENETDOWN** を返し、networkd・Settings・system bar へ「Network is down」として届く。
  host 試験で修正前に再現した（close の後 state=DOWN terminal=0）。
- 残る別の候補（この Phase では直していない）: AX211 の ioctl の入口（`intel-ax211.c` 約 7120 行）が recovery 中・runtime が止まった間に
  SIOCSWLANCONNECT を ENETDOWN で断る。前の操作の recovery の最中に join が来た場合はこちらで ENETDOWN になる。

## 実装（`src/kern/net/wifi/wlan.c` の `station_retire_controlled` だけ。driver・HAL は変えていない）

- 管理上も down にする retire（close・quiesce・detach・shutdown。`keep_administrative_up == 0`）で、station が既に FAILED で terminal_error を
  持っていれば、その理由を保つ（最初の失敗を terminal error にする。stop の失敗より前）。state は従来どおり DOWN。
- 明示の disconnect（`keep_administrative_up == 1`）は自分の generation を持つので、従来どおり自分の結果を報告する。
- `wlan_station_open` と次の connect は今までどおり terminal_error を 0 にする。

## 検証

- build: `make -j8 ZEDBSD_CONFIG=plan/ws089/tests/config-amd64-settings.mk BUILD=build/p1-q640 build/p1-q640/vmunix` → exit 0、warning 0、`amd64 vmunix check: PASS`。
- host: `sh plan/ws005/tests/host-wlan-retire.sh` → `host-wlan-retire: PASS`（共通の core を WLAN_TESTING で host に build、成功するだけの radio。
  EACCES の link loss → close の後も status が EACCES、open で消える、明示の disconnect は 0）。修正前の source（`REVERT=1 REVERT_COMMIT=<修正前>`）で
  `after the close: state=0 terminal=0` が FAIL（再現）。
- QEMU: T1 には出さない（AX211 は QEMU に無い。Q1 の指示）。
- 実機（S2）の確かめ方は [BUG-157](../../bugs/BUG-157.md) に書いた。未実施。
