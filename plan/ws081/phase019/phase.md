<!-- awesome-plan project=zedbsd record=ws081-p019 -->
# ws081-p019: PS/2 の mouse・touchpad の wheel（IntelliMouse の切替）（BUG-156）

Status: cleared（Q1 判定 2026-10-03: T1-015（QEMU）ps2-wheel-qemu PASS、id=4・wheel 上 3 下 2・動き・left・side。5330 のタッチパッドの 2 本指の scroll と `i8042: mouse id=` は S2 で確認）。元の記載: in-progress（実装済み・T1 の試験待ち）
Disposition: normal
Parent: [WS081](../ws.md)
Queue: q640（P1 generation11、2026-10-03。Q1 の dispatch。承認: user「実機がなくても修正できるバグをP1で修正、T1で順次テスト、のパイプラインを実行してください。」「実行してください。」）
Bug: [BUG-156](../../bugs/BUG-156.md)

## 範囲と原因

S1 の 5330 でタッチパッドの scroll が効かない（pointer・click は動く）。zedBSD に I2C HID の driver は無いので、pointer が動くことから
5330 の touchpad は firmware の PS/2 互換で `src/drivers/platform/pcat/ps2-8042.c` を通っている（推定、実機の log で確かめる）。この driver は
SET_DEFAULTS と ENABLE_STREAM だけで 3 byte の標準 packet を読み、IntelliMouse の切替（sample rate の knock）をせず REL_WHEEL を出さなかった。
PS/2 互換の touchpad の 2 本指・端の scroll は IntelliMouse の wheel として届くのが普通。WS006（入力）は completed なので、scroll の質を扱う
WS081 に置いた（Q1 の指示で所属は見立てで決めてよい）。

## 実装（`src/drivers/platform/pcat/ps2-8042.c`。HAL は変えていない）

- mouse の開始（`mouse_start`）で SET_DEFAULTS の後に `mouse_identify`: knock 200・100・80 → ID 3（IntelliMouse、4 byte）、続けて
  200・200・80 → ID 4（Explorer）。最後に sample rate を 100 に戻す。knock を断る mouse は今の ID の protocol で使う（ID も読めなければ従来どおり使わない）。
  kernel log `i8042: mouse id=N packet=M`。
- 4 byte 目の解釈: ID 3 は符号付き byte（REL_WHEEL = -Z）。ID 4 は Linux の psmouse と同じ（bit 7・6 が 10 で 6 bit の縦、01 で横、00・11 で
  4 bit の縦と side・extra の button）。capability に REL_WHEEL・REL_HWHEEL・BTN_SIDE・BTN_EXTRA を足した。
- 動きを 9 bit で読む（1 byte 目の bit 4・5 の符号）。今までは byte を int8 で読んでいたので、速い指の動きで 128 count 以上になると逆向きに飛んでいた。
- compositor（`userland/desktop/wayland/input.c`）は REL_WHEEL・REL_HWHEEL を既に axis にしているので変えていない。

## 2 本指の scroll について

PS/2 互換の touchpad が 2 本指の scroll を IntelliMouse の wheel として送るかは touchpad の firmware 次第で、読みでは確かめられない。送らない
（Synaptics・ALPS・Elantech の独自 protocol だけ）場合や、touchpad が I2C HID でしか scroll を出さない場合は、この修正では直らない（その時は止めて報告）。

## 検証

- build: `make -j8 ZEDBSD_CONFIG=plan/ws089/tests/config-amd64-settings.mk BUILD=build/p1-q640 build/p1-q640/vmunix` → exit 0、warning 0、`amd64 vmunix check: PASS`。
- host: `sh plan/ws081/tests/host-ps2-wheel.sh` → `host-ps2-wheel: PASS`（driver を 8042 と mouse の model で動かす。標準・IntelliMouse・Explorer の
  識別と packet size、rate の復帰、動き（9 bit の符号）、縦・横の wheel、side button、scroll の packet で side が保たれる）。
- QEMU（T1 に依頼）: `plan/ws081/tests/ps2-wheel-qemu.sh IMAGE BUILD`（usb-tablet の無い q35 の guest で PS/2 mouse だけにし、QMP の wheel 3 上・2 下、
  動き、左・side の click を guest の `ps2wheel`（evdev を読む probe）で数える）。未実施（結果待ち）。
- 実機（5330）は未実施。S2 で kernel log の `i8042: mouse id=` と 2 本指の scroll を見る。
