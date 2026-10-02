<!-- awesome-plan project=zedbsd record=ws129-p009 -->
# ws129-p009: デモの image を CI の設定を土台に作る

Status: in-progress（q594-i01）
Disposition: normal
Parent: [WS129](../ws.md)
Queue: q594 / q594-i01（P1）

## 範囲（2026-10-02 user「デモのイメージはCI設定をベースに変更しましょう。」）

1. デモの image の config（`plan/ws075/demo/build-demo-image.sh`、`config-demo-hdmi.mk` → `config-zdesktop-hw.mk` → `plan/ws035/tests/config-amd64-userland.mk`）を、CI の `config/ci/config-amd64.mk` を土台にして、デモに要る差分（display の boot line など）だけを重ねる形に変える。AX211 の driver と `intelax211-firmware` が CI と同じく揃う（BUG-134 の q590 の発見）。passthrough 用の VBT の選択（`passthrough` の語）は今の挙動を保つ。
2. 差分を表にする（旧デモ config と CI の config の package・driver・boot line の違い、残す差分の理由）。
3. q590 で見つかった `boot-test.sh` の QMP `screendump` の 30 秒 timeout（CI 土台の image、AX211=n でも同じ）を調べ、原因を特定して道具か image 側を直す（`plan/tools/boot-test.sh`・`boot-test.py` の修正は可）。
4. 新しいデモ image を build（warning 0）し、`boot-test.sh` で login prompt を確認。5330 の実機の単独起動はユーザーの確認（未実施と書く）。

## 所有 path

`plan/ws075/demo/`、`plan/ws035/tests/config-amd64-userland.mk` と demo が使う config、`plan/tools/boot-test.*`、`plan/ws129/phase009/`。`config/ci/` の CI の設定そのものは変えない（変えるなら Q1 に返す）。

## 受け入れ

上の 1〜4。古い demo の build directory は消さない。
