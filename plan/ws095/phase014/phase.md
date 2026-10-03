<!-- awesome-plan project=zedbsd record=ws095-p014 -->
# ws095-p014: IME の確定のたびの止まりと Text Editor の入力の遅れ（BUG-143）

Status: cleared（Q1 判定 2026-10-03: T1-024（QEMU）日本語の確定の後の IME の迂回 0→0、missed 0。辞書の保存の止まりの修正の範囲。BUG-143 の直接入力の遅れ（QEMU で textedit 337/416 ms、terminal 311/321 ms の median/max、host の IO が高い時の参考値）は未解決で BUG-143 に残す）。元の記載: in-progress（IME の止まりを修正・T1 の計測待ち。Text Editor の直接入力の遅れは原因未特定）
Disposition: normal
Parent: [WS095](../ws.md)
Bug: [BUG-143](../../bugs/BUG-143.md)
Queue: q643（P1 generation11、2026-10-03。user「優先のバグを解決し、そのあと実機なしで解決できるバグをどんどん処理してください。」）

## 範囲

1. IME の確定（変換を学んだ commit）のたびに 500 ms を超えて止まり、zdesktop が IME を迂回する（`ZWL IME bypass after_ms=500`）。
2. user の観察: Text Editor では IME を通さなくても全ての文字入力に約 500 ms の遅れがあるように見える（5330）。

## 1 の原因と修正

`ja-engine.c` の確定は、学んだ時に `ja_user_save` を key の経路の中で同期で呼んでいた（利用者の辞書の全体を一時 file に書き、fsync し、rename）。
UFS（QEMU、USB の stick）では fsync が遅く、その間 IME は key に答えず、zdesktop は 500 ms で IME を迂回して key を app に直に渡していた。

修正（`userland/desktop/ime/ja-user.c`・`ja.h`・`ja-engine.c`）: `ja_user_save_later` を足し、確定はこれを使う。辞書の text は key の経路で作る（memory の中、速い）。
書き込み・fsync・rename は専用の thread が行い、待っている text は新しい方が置き換える。辞書を閉じる時（`ja_user_free`、IME の終了）は thread の最後の
書き込みを待つ。`ja_user_save`（同期）は残し、thread の書き込みと一時 file を同時に書かないよう file の lock を共有する。Linux・FreeBSD の build に `-pthread`。

## 2 について（読みの結果）

直接入力（既定の "direct"）では zdesktop は key を IME に渡さない（`zwl_ime_key_grab` が direct で 0）ので、1 の止まりは直接入力の key には効かない。
Text Editor の main loop・libkeiui の dispatch（prepare_read と poll）・libwayland の reader の協調・WSI の event の読み方を読んだが、500 ms の
待ちを作る所は見つからなかった。最も近い周期は Text Editor の cursor の点滅（530 ms）。user が「あ」のまま英字を打っていた場合は 1 の経路で、
その時は今回の修正で消える見込み。計測の試験を作り T1 に依頼した。

## 検証

- host: `sh plan/ws095/tests/host-engine.sh` → 208 passed, 0 failed（ASan・UBSan。新しい 6 項目: 後回しの保存、新しい保存が待っている物を置き換える、
  free が書き込みを待つ、開き直して読める）。
- build: `make … BUILD=build/p1-q638 build/p1-q638/bin/keiland-ime` → exit 0、warning 0。
- QEMU（T1 に依頼）: `plan/ws095/tests/latency-bug143.sh`（新規。`type-latency.py` で key から画面の変化までを VNC の撮影で測る: Text Editor の直接入力、
  Terminal の直接入力（基準）、日本語で変換を確定した直後の key。日本語の 5 回で `ZWL IME bypass` が増えないこと）。未実施（結果待ち）。
- 実機（5330）は未実施。2 は T1 の数字（Text Editor と Terminal の差）を見て次を決める。
