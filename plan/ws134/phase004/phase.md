<!-- awesome-plan project=zedbsd record=ws134-p004 -->
# ws134-p004: システムモニターの操作（M3a）

Status: uncleared（q645 の後の Q1 の依頼、P2、2026-10-03。`interact.c` を書いたところでユーザーの指示でラップアップ。未配線・未試験）
Disposition: normal
Parent: [WS134](../ws.md)
設計: [design.md](../design.md) §3.9・§3.10・§5
依存: p003（同じ source に重ねる。p003 の uncleared の直しとは独立に進められる）

## 範囲

tap / click で plate をカードとして前へ（外の tap で戻す）、長押し・右 click で固定、1 本指の swipe・Shift+wheel・←→ で時間の範囲、
pinch（開く: 指の下の plate の detail、閉じる: overview）、2 本指の tap（detail と overview の交互）、状態コアの drag で少し回す（離すと戻る）、
Tab・Shift+Tab・Enter・Space・Esc・P の key の操作。pointer の位置で視差（p003 から）。`--calm`（p002 で入った）。

## 今の状態（2026-10-03、commit 1227ff647）

- `userland/desktop/monitor/app.h`: `struct sm_focus`（カードの plate・出る進み・pin・key の plate）、`struct sm_touch`（libkeiland の
  `keiland_gesture`、指 5 本の id と位置、2 本指の tap の記録、pinch の済み、drag と core の drag、pointer の押下、`core_turn`）、
  `sm_app` の `focus`・`touch`、`interact.c` の prototype（`sm_interact_open/close/event/tick`、`sm_plate_name`、`sm_set_range`）。
- `userland/desktop/monitor/interact.c`（新）: 上の範囲の全ての入力の解釈と log（`ZMON CARD open|close|pin`、`ZMON VIEW detail|overview`、
  `ZMON FOCUS`、`ZMON CORE turn=`、range は `sm_set_range` の `ZMON RANGE`）。gesture の時刻は `kui_clock_us()`（kui の `arrival_us` と同じ時計）。
  2 本指の tap は libkeiland に無いので app が判定（2 本目から 300 ms 以内に全部離す、8 px 以上動かない、pinch が働いていない）。
- 確かめ: `make ZEDBSD_CONFIG=plan/ws134/tests/config-amd64-monitor.mk BUILD=build/p2-p002-img build/p2-p002-img/bin/monitor` が
  `-Werror` で通る（`interact.c` は Makefile に未登録なので link されていない）。`interact.c` は zedBSD の clang で
  `-Wall -Wextra -Werror -fsyntax-only` が通る。style-check・Linux/FreeBSD の build・host 試験・QEMU は未実施。

## 再開の条件（残り）

1. `main.c`: `main_set_range` を `sm_set_range`（非 static）に改名して prototype を消す。`main_input` は RESIZE・CLOSE・Ctrl+Q だけを
   自分で扱い、他の event（←→ の key を含む）を `sm_interact_event(app, &event, main_now(app))` へ渡す（←→ の重複を消す）。
   `main_open` で `sm_interact_open`、`main_close` で `sm_interact_close`。loop で `sm_interact_tick` が 1 を返す間、または指・pointer が
   押されている間は `app->dirty = 1`（固定の時計でも long press が時刻で出るように）。
2. `scene.c`: カードの描画。overlay の暗さ 0.4×e、box は plate の箱から中央の 72%×64% へ補間（e = 1-(1-p)^3）、title・値・「Pinned」の chip・
   その plate の系列の chart（下から上がる）。CORES は core ごとの棒、EVENTS は一覧、STATE は rule ごとの level。key の plate には ice の縁。
3. `space.c`: 状態コアの方位に `app->touch.core_turn` を足す。
4. Makefile・Makefile.linux・Makefile.freebsd に `interact.c`。Linux と FreeBSD の `-fsyntax-only`、`style-check.py`。
5. 試験: `plan/ws134/tests/config-amd64-monitor.mk` に `CONFIG_INPUT_TEST_INJECT := y` と `touchinject`（試験用の image だけ）。
   `plan/ws134/tests/monitor-p004.sh`: touchinject の script（`size 1280 800` を出力の pixel に合わせ、触る前に `wait 2600`）で tap・長押し・
   swipe・pinch・2 本指の tap、guest.sh で key、`ZMON CARD/RANGE/VIEW/FOCUS/CORE` の log と PNG（card.png・pinned.png・overview.png）で判定。
6. build（warning 0）→ commit → Q1 に merge 依頼 → T1 に `monitor-p004.sh` を依頼。

## stub の項目

p002・p003 と同じ（design.md §1.5 の表のまま）。画面の値は全て sim か replay で、本物は hostname・CPU の数・uptime だけ。
この Phase は入力だけで、stub の項目は増減しない。
