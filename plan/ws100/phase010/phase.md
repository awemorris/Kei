<!-- awesome-plan project=zedbsd record=ws100-p010 -->
# ws100-p010: system bar の音量の click が自分の保存の読み戻しで戻される（BUG-153）

Status: cleared（Q1 判定 2026-10-03: T1-018（QEMU）volume-bug153・volume-p004・volume-p005 PASS。実機（5330 の HDA）の確認は S2）。元の記載: in-progress（実装済み・T1 の試験待ち）
Disposition: normal
Parent: [WS100](../ws.md)
Queue: q638（P1 generation11、2026-10-03。承認: user「次のセッションはP1とT1を起動、実機がなくても修正できるバグをP1で修正、T1で順次テスト、のパイプラインを実行してください。」「実行してください。」）
Bug: [BUG-153](../../bugs/BUG-153.md)

## 範囲と原因（読みで特定）

S1 の 5330 で slider の 50% を click しても次の瞬間に 100% になった。audiod・kernel の HDA・libkeiland の経路は set した値をそのまま報告する
（HDA は `controller->volume` の cache、soft の時は audiod の soft 値）ので、100 は zdesktop の中で作られている。

`volume.c` は変化の 1 秒後に `sound.volume`・`sound.muted` を desktop.conf に保存する。zdesktop の preferences の watcher（`preferences.c`、
別 thread、1 秒ごと）は自分の reading を持ち、zdesktop 自身の書き込みも「file が変わった」と見て、event loop が次の pass で reading を
差し替えて `preferences_apply` → `zwl_volume_preferences` → `volume_apply_preferences` を呼ぶ。これは保存の 0〜2 秒後に届き、その間に
user が別の値を click していると、file の古い値（例 100）が audiod の今の値（50）と違うので 100 を送り直していた。直前に 100 の側を
触っていれば「50 を click → 次の瞬間 100」になる。QEMU の volume-p004 は操作の間隔と確かめの時刻の偶然で通っていた見込み。

## 実装（`userland/desktop/wayland/volume.c`）

- file の変化からの適用（`zwl_volume_preferences`）は、drag 中・保存待ち（`save_ms != 0`）の間は飛ばす（手元の変化の方が新しい）。
- zdesktop 自身が最後に保存した値（`saved_value`・`saved_muted`）と同じ、または保存から `VOLUME_ECHO_MS`（3 秒）以内の変化は、自分の
  書き込みの読み戻しとして飛ばす（2 つの key を順に書くので途中の reading も含む）。log `ZWL VOLUME preferences skipped reason=newer|echo`。
- audiod に接続した時の適用（login・audiod の再接続）は今までどおり。Settings の Sound の頁は audiod に直接送るので、3 秒の窓で飛ばしても音量は追う。

## 検証

- build: `make -j8 ZEDBSD_CONFIG=plan/ws089/tests/config-amd64-settings.mk BUILD=build/p1-q638 build/p1-q638/bin/wayland` → exit 0、warning 0。
- host 試験: 無し（compositor の中、host の枠が無い）。読みで確かめた。
- QEMU（T1 に依頼）: `plan/ws100/tests/volume-bug153.sh`（新規。右端と中央を 1.3 秒おきに 5 往復 click、5 秒後に audiod が 49〜51・読み戻しの
  適用 0、手で書いた sound.volume=30 は適用される）と回帰の `volume-p004.sh`・`volume-p005.sh`。未実施（結果待ち）。修正前の image で
  volume-bug153 が FAIL するか（再現）は未実施。
- 実機（5330）は未実施。他の原因（実機の HID の wheel の誤読など）は否定できていない。実機で再発したら `ZWL VOLUME` の log で切り分ける。
