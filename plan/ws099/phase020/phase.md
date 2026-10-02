<!-- awesome-plan project=zedbsd record=ws099-p020 -->

# ws099-p020: BUG-125 の原因の特定と compositor の直し（ベータ1 の blocking）

Status: uncleared（q591-i01 は 2026-10-02 23:40 に時限で終了、P2。原因の特定と compositor の直しはできた。受け入れの C3・C4・C8・C9 は、直す前の image でも同じ試験が落ちていて未達）
Disposition: normal
Parent: [WS099](../ws.md)
Queue: [q591](../../queue.md) / q591-i01（承認: 2026-10-02 ユーザー「作業を開始しましょう。」、P2、時限 4h）
依存: [p017](../phase017/phase.md) の資産（q577・q583・q589 の証拠、`plan/ws099/tests/p017-popup-low-overhead.py`・`p017-popup-observe.py`・`p017-popup-timeline.py`、`phase017/p076-framed.sh`）。Venus の renderer `build/ws035-sq-venus/install`（読むだけ）
目安: 4h（1 Queue）。実行者の目安: phase-runner（high、compositor の bug の修正）
所有 path: `userland/desktop/wayland/` のうち原因の file（見込み: `shell.c`・`popup.c`・`menu.c`・`menu-shell.c`・`toplevel.c`・`seat.c`・`compose.c`）、`plan/ws035/tests/zdesktop-p076.sh`（試験の同期の直しが要るときだけ。main に一言）、`plan/ws099/tests/`・`plan/ws099/phase020/`、[BUG-125](../../bugs/BUG-125.md) と Bug Board の該当の行

## 背景

2026-10-02 user: BUG-125 は WS099 の blocking、WS099 の担当が直す。p017 は試験の同期の切り分けに限った Phase（compositor は読むだけ）で、
q577・q583・q589 の 3 attempt が uncleared。残る症状は 3 つ（[q577 結果](../phase017/q577-result.md)、[q583](../phase017/q583-result.md)、[q589](../phase017/q589-result.md)）:

1. popup: flip した submenu が configure・map されるが、最初の PNG が暗い（orange でない）。frame の log が無い。
2. popup: 負荷の下で flip した submenu の configure も press も無く、次の click で遠い menu が map される（準備前の操作）。
3. resize: left の resize の後の geometry が期待に届かない（WS105 q532 416×400 / x674、期待 200×400 / x890。q538 の C2 の settle の不一致とも近い）。

p017 の accepted-request の同期の修正（`fb8732b9`）は main に入っている。

## 範囲

1. 現在の main で基準の image を作り（`build-criteria-image.sh`）、q589 の low-overhead helper で p076 の stage 1〜4 を最大 5 回、全体の p076 を 5 回流し、3 症状の再現と再現率を得る。
   一時的に `--log-frames` と compositor の内部の印（configure・map・commit・最初の compose・resize の settled）を log に足してよい（取った後に外すか、既定で off の log にする）。
2. 各症状について、compositor の状態の遷移（xdg_popup の configure → ack → commit → map → compose、resize の request → configure → ack → settled → compose）のどこで遅れ・取りこぼしが起きるかを log の時刻で示す。
3. compositor の defect なら compositor を直す。試験が準備前に操作している（例えば popup の map の前の click）だけと証明できたら、試験に「map と最初の frame の log を待つ」同期を足す（待ちは最大 0.5 秒 × 6、期待の geometry・pixel は弱めない）。
4. 直した後の受け入れの試験を流し、BUG-125 を resolved にする（下の条件を全て満たしたときだけ）。p017 の受け入れ（単独 20・C9 5 で FAIL 0）もここで満たせば、main が p017 を follow-up で clear にできる。

範囲の外: C2 の top-right の増分・bottom-left の settle（p021）、BUG-127（p021）、BUG-120・BUG-124、速さ（C6）。

## 受け入れ

- 同じ image で `plan/ws035/tests/zdesktop-p076.sh` の単独 20 回が FAIL 0、`sh plan/ws099/tests/criteria.sh IMG OUT C9` の 5 回で p076 を含め FAIL 0（他の本の FAIL は別に記録し、新しい FAIL なら原因の見当を書く）。
- 3 症状のそれぞれに、原因（log の時刻と code の位置）と直し（または試験の同期だけで足りる証拠）がある。待ちを伸ばして隠していない（直しの前の image で同じ試験が FAIL を出すことを 1 回以上示す）。
- 変えた compositor の source の全文規約（[coding-style.md](../../coding-style.md)、`plan/tools/style-check.py` の違反 0）、`build/amd64/bin/wayland` の build の warning 0、C2・C3・C4・C8 の回帰 PASS、boot test PASS（PNG を Q1 経由でユーザーに見せる）。
- 4h で届かなければ uncleared。症状ごとの到達点・再開条件・次の Phase の案を残す。

## 検証の方法と範囲

- QEMU の Venus だけ（[guide.md](../guide.md)「コマンド」）。QEMU の console・serial の log では判定しない。判定は zdesktop の log（SSH、`/run/user/1000/session.log`・`/tmp/zdesktop.log`）・QMP の画面・試験の exit。
- runtime・port・build は担当の worktree の中に分ける（`build/<担当>-p020-*`）。共有の `build/` と toolchain は読むだけ。
- 実機は範囲の外（実機での resize は p011 の `resize-hw.sh` が既に PASS。必要になったら別 Phase）。

## 未決の判断

- なし（直し方は通常の技術の裁量）。compositor の外（libwayland・kernel の poll）に原因があると分かったら、その時点で止めて Q1 に報告する（他の WS の source）。

## Event

2026-10-02 / ws099-beta1-plan-p020: fg019 の計画で新設。p017 の 3 attempt の資産を引き継ぎ、compositor の編集を許す Phase として分けた。p017 の記録と受け入れは不変。

## 結果（q591-i01、P2、2026-10-02）

worktree `/home/awe/zedBSD-worktrees/p2`（branch `agent/p2`、base `901037f9f`）。QEMU 10.0.11 / zedBSD Venus（`build/ws035-sq-venus` を読むだけで使用）。
判定は zdesktop・probe の log（SSH で取得）、QMP/VNC の PNG、試験の exit で行い、console・serial の log は読んでいない。実機での確認はしていない。

### 原因（3 症状）

| 症状 | 原因（log の時刻、code の位置） | 直し |
| --- | --- | --- |
| 1. flip した submenu は map されるのに、最初の PNG に出ていない | compositor の event loop が `zwl_preferences_tick`（`preferences.c`）の `keiland_preferences_reload` → `stat(~/.config/keiland/desktop.conf)` で 0.7〜6.5 秒止まっていた。診断の build（loop の step ごとに 100 ms を超えた時間を出す。commit していない）で `ZWL DIAG slow step=preferences ms=6559` などを観測（[diag2-run1](evidence/diag2-run1-zdesktop.log)、[diag2-run2](evidence/diag2-run2-zdesktop.log)）。止まっている間は入力も frame も進まない。click が数秒遅れて client に届くので、試験の capture（click から 1.3 秒以上後）の時点では popup がまだ map されていなかった（[diag1-run3](evidence/diag1-run3-zdesktop.log): press は 896.953 s に処理され、その前の 8.8 秒間は入力の処理が無い。`PERF 6159ms: passes=20`） | stat を含む 1 秒ごとの確認を watcher thread（`preferences.c`）に移した。thread は自分用の読み取り結果を持ち、file が変わったら pending を立てる。event loop は mutex の下で pending を見て、読み取り結果を自分の物と入れ替えるだけで、disk を待たない。thread を作れないときは今までどおり loop で確認する |
| 2. 負荷の下で、flip した submenu の configure も press も無く、遠い menu が後で map される | 症状 1 と同じ stall。[diag1-run2](evidence/diag1-run2-zdesktop.log) では、遠い menu を開く click の press と release、次の click の release の 3 つが同じ時刻（847.528 s）にまとめて処理されている。その前の 5.6 秒間は何も処理されていない（`PERF 8370ms: passes=204`）。次の click の press は失われ、probe には遠い menu を開く press と release が 2 つしか届いていない（[probe](evidence/diag1-run2-probe.log)）。q577 の observation1（frame16 の完了まで 2660 ms、create/pong が frame17 の直後にまとめて届く）も同じ形 | 症状 1 と同じ |
| 3. left の resize の後の幾何が足りない（q532: 416×400 / x674） | client の move・resize の要求が遅れて届くと（症状 1 の stall や client の遅れ）、compositor は要求を受けた時点の pointer の位置を起点にしていた（`toplevel.c` の `zwl_toplevel_resize_start` の `resize_pointer_x = pointer_x`、`shell.c` の `drag_dx = pointer_x - surface->x`）。press から要求までの pointer の動きが失われる。q532 の 416 = 550 − (400 − 266) は、要求が 3 段の drag の 2 段目の後に届いた場合の値と一致する | press を client に届けた時の pointer の位置を `press_x/press_y`（`zwl.h`・`seat.c`）に記録し、client が要求した move・resize の起点にした（`toplevel.c` の `move_anchor`・`resize_anchor`）。press 以後の動きはすぐに反映する |

kernel 側の `stat` の遅さ（UFS、compositor の外）は Q1 が [BUG-135](../../bugs/BUG-135.md) として起票した（tracking）。この Phase では compositor 側の回避までを行い、kernel は直していない。

### 直しの前後の観測（QEMU）

| image / binary | 試験 | 結果 |
| --- | --- | --- |
| 直す前（`build/p2-p020-criteria`、main `901037f9f`、wayland sha256 `69e6c8d3…`） | 元のままの p076 全体 3 回 | 3/3 FAIL（flipped が 2b3444。[run1](evidence/base-orig-1.txt)・[run2](evidence/base-orig-2.txt)・[run3](evidence/base-orig-3.txt)・[PNG](evidence/base-orig-1-flipped.png)） |
| 直す前の wayland、および時刻の印だけを足した診断の build | p076 の段階 1〜4 と、その後に撮り直す試験（[script](evidence/p076-stage4-diag.sh)） | flipped の最初の PNG: 8 回中 4 回 FAIL（印を足した 4 回中 2 回、`--log-frames` 無しの 2 回中 1 回、step の時間を測る 2 回中 1 回）。`--log-frames` 付きの 1 回は PASS |
| 診断: `zwl_preferences_tick` を呼ばない build | 同じ | 5/5 PASS。PERF の窓は全て約 5 秒（stall 無し） |
| 修正（`f1af6cc7f` の wayland） | 同じ | 5/5 PASS。PERF の窓は全て約 5 秒 |
| 直す前の wayland | [p020-late-request](../tests/p020-late-request.sh)（probe が要求を 400 ms 遅らせ、drag は press と同時に始める） | FAIL（stall で release が先に処理され `refused=no-press`。[log](evidence/late-base-1.txt)） |
| preferences の直しだけで、起点の直しが無い build | 同じ | 2/2 FAIL（move は受け付けられるが移動量が 0: `GLASS moved x=440 y=293`。[log](evidence/late-noanchor-1-zdesktop.log)） |
| 修正 | 同じ | 2/2 PASS（[1](evidence/late-fix-1.txt)・[2](evidence/late-fix-2.txt)） |

p076 の試験の同期（map と最初の frame の log を待つ）は案を作って試したが入れていない（`--log-frames` を付けると時間の関係が変わり、症状が出にくくなるため。不具合を隠さないよう、受け入れには元のままの p076 を使う）。

### 受け入れの試験（直した image `build/p2-p020-fixed`、hdd-image sha256 `aff65361…`、wayland sha256 `c731421a…`）

- build: `build-criteria-image.sh build/p2-p020-fixed` は exit 0。compiler の warning は 0（log の warning は gmake の jobserver の 1 行だけ）。`bin/wayland`・`bin/popup-probe` を単独で build しても warning 0。
- 規約: 変えた C の file（`preferences.c`・`toplevel.c`・`seat.c`・`zwl.h`・`popup-probe/main.c`）で `plan/tools/style-check.py` の違反 0。coding-style.md の全文で手で見直した（critical section の段落、forward declaration、file scope の変数と型の comment、関数の中の呼び出しを条件に置かない、など）。`git diff --check` も PASS。
- boot test: PASS（[PNG](evidence/boot-login.png)、framebuffer の login prompt）。base の boot-test には QMP の timeout の不具合がある（main `a768b804c` で修正済み）。そのため main の修正版を `plan/ws099/temp/` に複写し、root の path だけを直して実行した。
- p076 単独 20 回（1 回目、元の harness）: PASS 13 / FAIL 7（[summary](evidence/acc20/summary.txt)）。FAIL の 7 回は全て harness の SSH の失敗で、count を数える ssh が status 255 で落ちた（うち 1 回は `Connection timed out during banner exchange`）。どの回も compositor は要求を受け付けている（[例](evidence/acc20/p076-4-zdesktop.log)）。画素と log の判定で落ちた回は 0。
- 同じ guest で直す前と直した後の compositor を交互に 4 巡流した比較（[summary](evidence/sshcmp/summary.txt)）: SSH の失敗は直す前に 3+0+2+0 回、直した後に 2+0+2+0 回で、どちらにも banner timeout が出る。直す前の 4 巡目は flipped の画素で FAIL した。SSH の stall は今回の変更が原因ではなく、guest の sshd が 5 秒以上応答しないため（ConnectTimeout=5）。BUG-135（UFS の stat の遅さ）と同じ根の可能性があるが、証明していない。
- Q1 の判断（2026-10-02）: sshd の stall は compositor の範囲外。harness の count の SSH に retry を入れ、SSH の失敗は harness の失敗として数え直し、画素と log の判定で FAIL 0 を受け入れの基準にする。これを受けて `plan/ws035/tests/zdesktop-p076.sh` に `guest_retry` を足した（ssh の status 255 のときだけ、合わせて 3 回まで試す。1 回ごとに host の deadline を付け、retry は stderr に出す）。期待の幾何・画素・log の判定は変えていない。
- p076 単独 20 回（2 回目、`guest_retry` 付き、[summary](evidence/acc20b/summary.txt)）: PASS 19 / FAIL 1。20 回のうち 9 回で SSH の retry が起きた（banner timeout が計 30 回。いずれも 2 回目以内の試行で回復）。FAIL の run 19 は probe が compositor につながらなかった（`POPUPPROBE FAILED run=p setup errno=5`、[probe](evidence/acc20b/p076-19-probe.log)）ため、全ての段が連鎖して落ちた。直す前の compositor でも同じ形の FAIL が出ている（[sshcmp base-1](evidence/sshcmp/base-1.txt)）。直前に banner timeout が出ているので、guest の stall で zdesktop の起動が 15 秒の待ちを超えたと見ているが、未証明。compositor が落ちたことを示す証拠も無い（その回の zdesktop の log は filter され、次の回で上書きされた）。
- C2: PASS（14/14、[結果](evidence/critcmp/concurrent-fix-results.txt)。p076 と並行で流した回）。
- C3（p138・c3-swipe-back）、C4（p137）、C8（p134）、C9 の p072: 直した image で FAIL（[fix](evidence/critcmp/fix-results.txt)）。ただし同じ試験は直す前の image（main `901037f9f`）でも同じ所で FAIL した（[base](evidence/critcmp/base-results.txt)）。p138・c3 は Notes の窓に title bar が出ず、title bar の drag で `GLASS moved` が出ない（[PNG](evidence/critcmp/fix-p138-moved.png)、[base の log](evidence/critcmp/base-p138.log)）。p137 は probe-a の focus と key。p134 は probe の本体の角。どれも今回の変更の前から base にある失敗で、BUG-125 の範囲の外。title bar が無い症状は BUG-136・BUG-137（次の p023）と関係する可能性がある。
- C9 の p052: 直す前は FAIL、直した後は PASS（1 回ずつ）。C9 の全体を 5 回は未実施（時限）。並行で流した 1 回目（[concurrent](evidence/critcmp/concurrent-fix-results.txt)）は p076 の 20 回と同時に 2 つの guest を動かしたため、判定には使わない。

### 未実施・残り

- C9 の全体 5 回は未実施。C3・C4・C8・C9 の p072 が base で既に落ちているので、FAIL 0 は BUG-125 の直しだけでは満たせない。それぞれ別の Phase（p023・p021 など）か bug の判断が要る。
- 実機（5330）での確認は未実施（範囲の外）。
- BUG-135（kernel の stat・sshd の stall）は直していない（Q1 の判断）。
- run 19 の setup の失敗（probe が接続できない）は原因が未確定。zdesktop の起動時の log を残す harness の改善が要る。
- `plan/ws035/tests/zdesktop-p076.sh` の SSH の retry は、他の C9 の試験（p137・p138 など）には入れていない。

### 再開の条件と次の Phase の案

- compositor の直し（`f1af6cc7f`）と harness の retry（`a781d60f5`）を main が統合する。そのうえで、base の C3・C4・C8・C9 の FAIL（title bar が出ない、p137 の focus、p134 の角、p072）を BUG-136/137 と合わせて p023 か別の bug で扱った後に、C9 の 5 回を流し直す。
- BUG-125 の 3 症状は、原因と直しの証拠がそろった。resolved にするかは Q1 が判断する（p076 単独の受け入れは、harness の失敗を除いて 19/19 が PASS）。
