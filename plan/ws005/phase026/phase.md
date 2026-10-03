<!-- awesome-plan project=zedbsd record=ws005-p026 -->
# ws005-p026: WiFi の join の間「Connecting...」を出す（BUG-154）

Status: cleared（Q1 判定 2026-10-03: T1-013（QEMU）connecting-bug154・settings-p003・zdesktop-p013 PASS。実機は S2。強調行の「Connecting...」の色の読みにくさは P1 が続けて直す）。元の記載: in-progress（実装済み・T1 の試験待ち）
Disposition: normal
Parent: [WS005](../ws.md)
Queue: q638（P1 generation11、2026-10-03。承認: user「次のセッションはP1とT1を起動、実機がなくても修正できるバグをP1で修正、T1で順次テスト、のパイプラインを実行してください。」「実行してください。」）
Bug: [BUG-154](../../bugs/BUG-154.md)

## 範囲

S1 の実機で別の AP を選ぶと約 10 秒何も出ず、その後にチェックが付いた。user「これは問題なので修正が必要。Connecting...は必要。」
原因（読み）: system bar の menu は開くと scan を出し、その間に押した join は slot で待つ。join の間も networkd の state は前の AP の
connected のままで、menu は daemon の state（`KL_BACKEND_WIFI_CONNECTING`）でしか「Joining」を出さなかった。Settings の行の「Connecting...」も
join が送られてから（`join_step`）で、slot で待つ間は出なかった。

## 実装

- `userland/desktop/wayland/network.c`（system bar）: `network_view.connecting`（選んだ AP、click から join の答えまで）。switch の下の行は
  「Connecting to X...」、その AP の行は右に「Connecting...」（padlock と電波の代わり）、その間は前の AP の check を出さない。daemon が自分で
  join している（CONNECTING）AP の行も同じ表示。鍵を入れた join（PROFILES → JOIN）も保存の時点から。join の答え（成否とも）・disconnect・
  Wi-Fi off・送れなかった join で消す。log `ZWL NETWORK connecting ssid=`。
- `userland/desktop/settings/page-network.c`: slot で待つ join（`pending_step`）の行も「Connecting...」。`network.c` の文言を
  「Joining X...」→「Connecting to X...」（user の言葉に合わせる。host 試験の期待も直した）。
- `userland/tests/network-probe/main.c`: 2 つ目の引数 JOIN-SECONDS（join の答えを遅らせ、その間 state は前のまま）。既定 0 で既存の試験は不変。
- 試験: `plan/ws005/tests/connecting-bug154.sh`（新規。system bar の join と切替、Settings の join で Connecting の行と PNG）。

## 検証

- build: `make -j8 ZEDBSD_CONFIG=plan/ws089/tests/config-amd64-settings.mk BUILD=build/p1-q638 build/p1-q638/bin/wayland build/p1-q638/bin/settings build/p1-q638/bin/network-probe` → exit 0、warning 0。
- host: `sh plan/ws089/tests/host-slot.sh` → `host-slot: PASS`（文言の期待を直した）。`sh plan/ws089/tests/host-build.sh` → built。
- QEMU（T1 に依頼）: `connecting-bug154.sh`、回帰に `plan/ws035/tests/zdesktop-p013.sh` 相当（system bar の menu）と `settings-p003.sh`。未実施（結果待ち）。
- 実機（5330 の AX211）は未実施。
