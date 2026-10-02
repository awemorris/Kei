<!-- awesome-plan project=zedbsd record=ws094-guide -->

# WS094 作業の手引き（2026-10-01）

この file だけで WS094（desktop の file の icon）を続けられるように、ゴール・今の状態・次の作業・未知・command をまとめた。
記録の正は [ws.md](ws.md)・[design.md](design.md)・各 phase.md で、ここはその要約と手順である（食い違えば ws.md と phase.md が正）。
2026-10-01 にこの手引きを書いた agent が確かめたこと: host 試験 3 本（下の §5.2）を main の checkout で流して PASS、script の行番号。
QEMU・実機・image の build は流していない（未確認）。

## 1. ゴール

### 1.1 デモの場面（fg010、[master.md](../master.md) の「fg010 の達成基準」）

| 場面 | 内容 | WS094 の分 |
| --- | --- | --- |
| S2 | kei で login、デスクトップ（壁紙・system bar・**デスクトップの icon**） | `~/Desktop` の icon が右上から並ぶ。double click で開く |
| S4・S5・S6 | Files・画像・text | desktop の icon からも Files・Image Viewer・Text Editor が開く（WS093 の対応） |

デモの実機（Dell Latitude 5330）は mouse と keyboard。touch の場面は Windows の host の QEMU で見せる（master の「デモの touch」2026-09-30）。

### 1.2 WS の完了の条件（design §7 の受け入れ、Phase の全て）

| # | 条件 | 状態 |
| --- | --- | --- |
| W1 | session の compositor で desktop に `~/Desktop` の icon が右上から並ぶ | 済み（p002・p003、QEMU） |
| W2 | double click で WS093 の対応の app が開く、folder は新しい Files の窓 | 済み（p004） |
| W3 | 右 click の menu・名前の変更・Trash・Copy・Paste・New Folder・Show in Files | 済み（p005） |
| W4 | 移動と保存、Files の窓との DnD、touch の double tap と long press | 済み（p006） |
| W5 | 段 L3〜L5 のうち main が選んだ段の Phase（ws.md「段」） | L3 は p008・p013 cleared、p009 uncleared（合否は実機、下の §2.2）。L4a p010 cleared。L4b p011 は q582でcleared（2026-10-02、[結果](phase011/q582-result.md)）、L5 p012 は planned |
| W6 | 全文の規約と回帰（p007、code を作る WS の最後の Phase。Awesome Plan §6） | planned |

**完了の定義**: W1〜W4 は済み。残りは「main が選んだ最後の段の Phase」と p007 が cleared になること。このあと ws.md を完了の形に書き直し、
Phase の directory と WS094 だけの試験を消し、後も使う試験（`files-desktop-guest.sh`・`host-desktop.*`・`host-thumb.*`）を `plan/tools/files/` へ移して
master の Tools 節に登録する（AGENTS.md「記録の置き場所」。master の Tools 節の編集は main）。
**WS094 の完了は WS090 の p009（Files を libkeiui へ移す）の前提**（`plan/ws090/ws.md` の Phase の表「WS094 の完了」）なので、長く止めない。

### 1.3 段の数値目標（ws.md「段（L1〜L5）」）

| 段 | 目標 | 測り方 |
| --- | --- | --- |
| L3 | 100 項目で (a) `files --desktop` の起動 → `DESKTOP ready` ≤ 1500 ms、(b) 1 つ足して表示 ≤ 2500 ms、(c) click → 選択の frame ≤ 50 ms、`SLOW-FRAME` 0 | `files-desktop-guest.sh … perf100`（3 回の中央値）。**合否は 5330 の実機（p012）、QEMU の値は参考**（Q1 の判断 2026-09-30） |
| L4 | (a) 40 文字の名前が 2 行で中を省く、(b) 画面の大きさを変えても保存の場所を保つ（p010 済み）。(c) grid より多い項目は描かずに数を log、保存の行の無い名前は次の保存で消える（p011） | host 試験と guest の log・画面 |
| L5 | 実機で L1 の手順と L3 の (a)(c) が目標以内 | p012（下の §3） |

## 2. 今の状態

### 2.1 済んだもの（証拠）

| Phase | 結果 | 証拠 |
| --- | --- | --- |
| p001 | 設計（J1〜J7 は既定: Files が背景の層の client） | [design.md](design.md) |
| p002 | compositor の `keiland_desktop_v1`、probe | [phase002](phase002/phase.md)、`build/ws094-shots/p002/`（worktree） |
| p003 | libkeiland の API、`files --desktop` | [phase003](phase003/phase.md) |
| p004 | 選択・開く・keyboard・配置の保存・Clean Up | [phase004](phase004/phase.md) |
| p005 | context menu・名前の変更・Trash・Copy・Paste・New Folder・Show in Files | [phase005](phase005/phase.md) |
| p006 | drag・DnD・touch | [phase006](phase006/phase.md) |
| p008 | L3 の計測の道具（`perf100`）と基準値 (a) 2908・(b) 1094・(c) 95 ms | [phase008](phase008/phase.md) |
| p013 | jpg・gif の thumbnail（共有の decoder `userland/desktop/picture/`） | [phase013](phase013/phase.md) |
| p010 | 長い名前の 2 行・画面の大きさの変更（統合の試験 `plan/ws079/tests/demo-s8-s9.sh` で合格扱い、ユーザーの指示） | [phase010](phase010/phase.md) |

2026-10-01 に main の checkout（fb1f3317）で流した host 試験: `host-desktop.sh` PASS（12 秒）、`host-thumb.sh` PASS（12 秒）、`plan/tools/files/host-model.sh` PASS。
（`plan/ws104/phase007/phase.md` の survey は「host-desktop は font の確かめ 2 つで落ちる」と書くが、main の checkout では再現しなかった。WS104 の copy の tree の差と見られる。未確認。）

### 2.2 開いているもの

| 項目 | 状態・再開の条件 |
| --- | --- |
| p009（L3b） | uncleared。QEMU で (a) 1954・(c) 90 ms。残りは Files の外（zdesktop の buffer の import と QEMU の Venus の 1 回約 10 ms）。**Q1 の判断: 合否は実機（p012）で決める**。zdesktop の import の短縮は WS099 の p016 へ移した。Venus の 10 ms は Future Work の F-064 |
| p010 の残り | Terminal を開く場面と ws035-p088 の切り分け、実機は未実施（phase010 の最後の節）。p007 の回帰で拾う |
| p011（L4b） | cleared（2026-10-02、q582）。host/build/guest/boot PASS（[結果](phase011/q582-result.md)） |
| p012（L5） | planned、実機とユーザーの時間が要る（[phase012](phase012/phase.md)） |
| p007（規約と回帰） | planned（[phase007](phase007/phase.md)） |
| p010 の L4 (b) の表の数値 | ws.md の段の表は「1920x1280」、p010 は 1920x1080 で試した。どちらでも規則は同じ（保存の場所が grid の中なら保つ） |

### 2.3 既知の bug

| bug | 内容 | 状態 |
| --- | --- | --- |
| [BUG-123](../bugs/BUG-123.md) | desktop-probe が `--timeout-s=3` の後に終わらない（`desktop-guest.sh` の restart の手順、p002・p006 で観察） | resolved（ws073-p044: libwayland-client の dispatch の待ち）。p007 の回帰で restart の手順が PASS になることを確かめる |
| [BUG-115](../bugs/BUG-115.md) | zdesktop-p072 の最初の接続の errno=5（ws094-p002 の回帰で発見） | resolved（ws099-p003） |
| F-064（[future-work.md](../future-work.md)） | QEMU の Venus の同期の呼び出しが約 10 ms の刻み | Future Work。WS094 の合否には使わない |

WS094 に開いた bug は無い（2026-10-01 の [known-bugs.md](../known-bugs.md)）。

## 3. 次の作業の順番

master の優先順位（2026-09-30 夜ユーザー）: WS099・WS079・WS090・WS089・**WS094**・WS100・WS078・WS102。2026-10-10 ごろまで新規実装、その後は bug と実機の調整だけ
（master「デモまでの期間」）。ユーザーの判断「溢れた icon は今のまま」（2026-09-30）を守る。

| 順 | Phase | 目的 | 実行 | 受け入れ |
| --- | --- | --- | --- | --- |
| 済 | [ws094-p011](phase011/phase.md) | L4b: 溢れた項目の数の log と、無い名前の保存の行の掃除 | agent（phase-runner-mid） | host 試験の新しい確かめ、guest の `files-desktop-guest.sh … install show saved prune`（新しい手順）PASS |
| 2 | [ws094-p012](phase012/phase.md) | L5: 5330 で L1 の手順と L3 の (a)(c) | ユーザーが 5330 を操作し、agent が SSH で log を読む | (a) ≤ 1500 ms・(c) ≤ 50 ms・SLOW-FRAME 0、L1 の操作が働く（写真） |
| 3 | [ws094-p007](phase007/phase.md) | 全文の規約と回帰（WS の最後） | agent（phase-runner） | style-check 0（既存の例外を除く）、host・guest・C9・boot test PASS |
| 4 | p009 の扱い | p012 の実機の値で (a)(c) が以内なら p009 を「実機で clear」の follow-up に、超えれば新しい Phase（実機の値を見て Files の command buffer の事前の記録、phase009「残り」の 3） | main の判断 | — |

- p012 が実機の都合で 2026-10-10 までにできないときは、p007 を先に行い、p012 を WS の完了の条件から外す（Future Work か fg010 の実機の確認の一覧へ移す）かを
  **main が決める**（Awesome Plan §7: 延期で受け入れを黙って消さない）。
- WS090 の p009・p010（Files を libkeiui へ）と WS094 の Files の Phase を重ねない（design §5、J5）。WS104 の p007（install の path の macro、
  `userland/desktop/files/` を含む 26 file）とも重ねない。重なるときは main が順を決める。
- 新しい Phase の提案（必要になったとき）: **ws094-p014**（提案）: 実機の値が目標を超えたときの Files の frame の Vulkan の呼び出しの削減
  （image ごとの command buffer を前もって記録、phase009「残り」の 3）。触る file: `userland/desktop/files/present.c`・`main.c`。受け入れ: 実機の (c) ≤ 50 ms。

## 4. 未知と調べ方

| 未知 | なぜ重要 | 調べ方 |
| --- | --- | --- |
| 5330 の実機での (a)(c) | L3 の合否は実機（Q1 の判断） | p012 の手順。session の log `/run/user/1000/session.log`（`userland/desktop/sessiond/session.c:589` が `$XDG_RUNTIME_DIR/session.log` に書く）を SSH で読む。Files の行 `ZFILES DESKTOP ready … at_ms=`（`userland/desktop/files/ui-desktop.c:195`）、zdesktop の行 `ZWL DESKTOP start … at_ms=`（`userland/desktop/wayland/desktop.c:738`）と `ZWL DESKTOP drawn at_ms=`（同 322）、選択の `ZFILES DESKTOP select-frame ms=`（`files/main.c:645`）、`SLOW-FRAME`（同 653） |
| 保存の行の掃除の正しい時点 | `fm_desktop_layout_set`（`files/desktop-layout.c:437`）は今の `desk->saved` を全部書き、無い名前の行が残る（2026-10-01 に code で確かめた）。`desk->shown`（`fm_desktop_remember`、同 556）は cell の無い（溢れた）項目を含まないので、掃除の基準に使うと溢れた項目の保存の場所を誤って消す | p011 の手順 1。listing の名前の配列（`ui-desktop.c:632` の `fm_desktop_arrange(names, tab->listing.count, …)` の `names`）で照らす。listing の読み込みが失敗した時は掃除しない |
| 溢れた数の log | 今は `DESKTOP ready items=100 cells=91`（`ui-desktop.c:195`）で items − cells が溢れた数。目標 L4 (c) の「数を log」はこれで足りるか、`hidden=` を足すか | p011 で `hidden=` を足す（既存の試験の pattern は `ready items=N cells=M width=` なので、`hidden=` は行の末尾に足す） |
| desktop-probe の restart（BUG-123 の後） | p006 で FAIL した手順が直ったか | p007 の回帰で `desktop-guest.sh … install role refuse input home-drag dnd restart` を 2 回流す（probe は `build-probe.sh` で作る） |
| Terminal と ws035-p088 の切り分け（p010 の残り） | p010 の統合の試験は Terminal を開かない | p007 の回帰に WS099 の C9（`criteria.sh … C9`）を入れる。C9 は WS035 の p052〜p138 の 10 本 |

判定は log（SSH）・画面（PNG）・終了の code だけで行う。**QEMU の console・serial の log を読んで判定しない**（AGENTS.md「検証」）。
guest の操作は `plan/tools/guest/guest.py run`（SSH）と QMP（`plan/ws035/tests/qmp-pointer.py`・`qmp-keys.py`）。

## 5. コマンド（repo の root から。`<W>` は作業の名前、例 `ws094-p011`）

共通の決まりは [plan/ws104/commands.md](../ws104/commands.md) の §0（image の build を 2 つ同時にしない・BUILD を必ず渡す・boot test の OUTPUT は専用）、
§1（warning の数え方）、§4（boot test）、§5（WS099 の基準 C9）を見る。ここは WS094 に固有のものだけ。

### 5.1 image の build（WS094 の試験の image）

Files・compositor・touchinject（touch の手順）・guest の harness の鍵を持つ image は WS102 の inset の image（WS079 の demo の image + Text Editor）。p010 もこれを使った。

```
mkdir -p build/<W>
sh plan/ws102/tests/build-inset-image.sh build/<W>-inset > build/<W>/inset-build.log 2>&1; echo "exit=$?"
ls -la build/<W>-inset/hdd-image.img
```

- PASS: `exit=0`、image がある。**引数を省くと build/amd64 に作り既定の image を上書きする**（`plan/ws102/tests/build-inset-image.sh:13` の `build=${1:-build/amd64}`）ので必ず渡す。
- source を直した後は、image を作り直さず program だけ作って guest に入れてよい（`files-desktop-guest.sh` の `install` が `$BIN/bin/wayland`・`files` と
  `$BIN/dynamic/*.so`（libc・ld を除く）を guest に put する、同 script の 98〜118 行）:

```
make -j64 ZEDBSD_CONFIG=plan/ws102/tests/config-amd64-inset.mk BUILD=build/<W>-inset build/<W>-inset/bin/files build/<W>-inset/bin/wayland build/<W>-inset/dynamic/libkeiland.so > build/<W>/bin-build.log 2>&1; echo "make exit=$?"
```

- 前の Phase の記録は `BUILD=build/amd64` や `config-amd64-zdesktop.mk` で作っている（p008・p009・p013）。**build/amd64 は他の WS と共有なので使わない。**
- 古い記録の image `build/ws081/demo-win-venus.img` は main の checkout に無い（2026-10-01 確かめ）。

### 5.2 host 試験（GPU・QEMU 不要、各 10〜15 秒）

```
sh plan/ws094/tests/host-desktop.sh; echo "exit=$?"
sh plan/ws094/tests/host-thumb.sh build/<W>/host-thumb; echo "exit=$?"
sh plan/tools/files/host-model.sh; echo "exit=$?"
```

- PASS: 最後の行が `host-desktop: PASS`・`host-thumb: PASS`・`files-model: PASS`、exit 0（2026-10-01 に main の checkout で確かめた）。
- どれも `plan/tools/files/host-build.sh` で `build/ws071-host/` に Files の host の object を作る（共有の directory。同時に 2 つ流さない）。

### 5.3 QEMU の guest 試験（Venus）

host の準備（host の起動ごとに 1 回）: `sudo modprobe vgem && sudo chmod 0666 /dev/dri/renderD128`（`plan/ws035/tests/zdesktop-guest.sh` の先頭の注釈）。

```
export GUEST_RUNTIME=$PWD/build/<W>-run
sh plan/ws035/tests/zdesktop-guest.sh start build/<W>-inset/hdd-image.img
sh plan/ws035/tests/zdesktop-guest.sh wait --timeout 240
BIN=build/<W>-inset sh plan/ws094/tests/files-desktop-guest.sh build/<W>/l1 install show watch input saved
BIN=build/<W>-inset sh plan/ws094/tests/files-desktop-guest.sh build/<W>/menu install show menu
BIN=build/<W>-inset sh plan/ws094/tests/files-desktop-guest.sh build/<W>/drag install show drag
BIN=build/<W>-inset sh plan/ws094/tests/files-desktop-guest.sh build/<W>/touch install show touch
BIN=build/<W>-inset sh plan/ws094/tests/files-desktop-guest.sh build/<W>/perf install perf100
sh plan/ws035/tests/zdesktop-guest.sh stop
unset GUEST_RUNTIME
```

- PASS: 各 run の最後の行 `files-desktop-guest: PASS`（`files-desktop-guest.sh:505`）。`perf100` は `build/<W>/perf/perf100.txt` の `RESULT` と `L3 …` の行に
  値と目標（`files-desktop-guest.sh:488` 以下）。QEMU の値は参考で、FAIL の行が出ても合否にしない（§1.3）。
- 手順の一覧と中身は `files-desktop-guest.sh` の先頭の注釈（2〜33 行）。`install` を各 run の先頭に置く（HOME `/tmp/dhome` の Desktop を作り直す）。
- 時間: 未計測（各 run 数分の見込み）。`zdesktop-guest.sh start` はすぐ戻るので `wait` を挟む。
- GUEST_RUNTIME を必ず渡す。既定は script ごとに違う: `files-desktop-guest.sh`・`desktop-guest.sh` は `build/ws094-run`（`files-desktop-guest.sh:38`・`desktop-guest.sh:23`）、`desktop-p010.sh` は `build/ws094-p010-run`（`desktop-p010.sh:21`）、`zdesktop-guest.sh` は `build/ws035-sq-run`。
  `zdesktop-guest.sh start` は最後の runtime を共有の file `build/.zdesktop-guest-runtime` に書き、GUEST_RUNTIME の無い `stop` はそれを止める。

compositor の probe の試験（compositor を変えた時と p007）:

```
sh plan/ws094/tests/build-probe.sh build/<W>-inset
export GUEST_RUNTIME=$PWD/build/<W>-run
sh plan/ws035/tests/zdesktop-guest.sh start build/<W>-inset/hdd-image.img
sh plan/ws035/tests/zdesktop-guest.sh wait --timeout 240
BIN=build/<W>-inset sh plan/ws094/tests/desktop-guest.sh build/<W>/probe install role refuse input home-drag dnd restart
sh plan/ws035/tests/zdesktop-guest.sh stop
unset GUEST_RUNTIME
```

- `build-probe.sh` は `build/amd64/sysroot`（共有の sysroot、読むだけ）と `BUILD/dynamic` で `BUILD/ws094/desktop-probe` を作る（`build-probe.sh` 9〜21 行）。
  PASS の行は `desktop-guest: PASS`（`desktop-guest.sh:209`）。

長い名前と画面の大きさ（p010、guest を自分で 3 回起こす）:

```
sh plan/ws094/tests/desktop-p010.sh build/<W>-inset/hdd-image.img build/<W>/p010
```

- PASS: `desktop-p010: PASS`。runtime は `build/ws094-p010-run`（上書きは `GUEST_RUNTIME`）。

### 5.4 回帰の組（p007 と、Files・compositor を変えた Phase の終わり）

| 何を変えたか | 流すもの |
| --- | --- |
| Files の desktop の部分だけ | §5.2 の 3 本、§5.3 の l1・menu・drag、boot test |
| Files の共有の部分（`ui-grid.c`・`thumb.c`・`present.c` 等） | 上に加え Files の guest 試験 14 本（下の files-regress） |
| compositor（`userland/desktop/wayland/`） | 上に加え probe の試験、WS099 の C9（commands.md §5 の `criteria.sh … C9`） |
| WS の最後（p007） | 全部 + touch・perf100・p010 |

Files の guest 試験 14 本（lean な files の image。guest を起こしてから流す。`plan/tools/files/files-regress.sh:5`・`files-guest.sh:5`）:

```
sh plan/tools/files/build-files-image.sh build/<W>-files > build/<W>/files-build.log 2>&1; echo "exit=$?"
GUEST_RUNTIME=$PWD/build/<W>-files-run sh plan/tools/files/files-guest.sh start build/<W>-files/hdd-image.img
GUEST_RUNTIME=$PWD/build/<W>-files-run sh plan/tools/files/files-guest.sh wait --timeout 240
GUEST_RUNTIME=$PWD/build/<W>-files-run sh plan/tools/files/files-regress.sh build/<W>/files-regress
GUEST_RUNTIME=$PWD/build/<W>-files-run sh plan/tools/files/files-guest.sh stop
```

- PASS: 最後の行 `files-regress: PASS (…)`（`files-regress.sh:19`）。各試験は 600 秒で打ち切り。`build-files-image.sh`・`files-guest.sh` も引数を省くと
  build/amd64 を使うので必ず渡す。`files-p0*.sh` は全て `GUEST_RUNTIME` を見る（既定 `build/ws071-run`、2026-10-01 に grep で確かめた）。

boot test（commands.md §4 と同じ。image は自分の inset の image）:

```
OUTPUT=build/<W>/boot plan/tools/boot-test.sh build/<W>-inset/hdd-image.img; echo "exit=$?"
```

- PASS: `exit=0` と `boot-test: PASS build/<W>/boot/login.png`。PNG をユーザーに見せる。

## 6. 実機（Dell Latitude 5330）

一般の手順は [plan/tools/hw5330/README.md](../tools/hw5330/README.md)（2026-10-01）。素の 5330（USB から単独の起動、ユーザーが行う）は README §2・§3、
passthrough（5330 の Linux の上の QEMU に iGPU を渡す、agent が centris から操作）は §1・§4。L5 の合否は**素の 5330**で取る。passthrough の値は
本物の i915 の参考として先に取ってよい（QEMU の証拠として分けて書く）。WS094 に固有の確かめ（p012）:

| # | 確かめ | 方法 | 判定 |
| --- | --- | --- | --- |
| H1 | login の後、desktop に `~/Desktop` の icon が右上から並ぶ | ユーザーの目・写真 | icon と名前が見える |
| H2 | double click で開く（txt → Text Editor、png・jpg → Image Viewer、folder → Files） | ユーザーの操作 | 開く |
| H3 | 右 click の menu、名前の変更、Trash、New Folder | ユーザーの操作 | 働く |
| H4 | (a) 100 項目で起動 → ready | SSH で log を読む（p012 の手順） | 中央値 ≤ 1500 ms |
| H5 | (c) click → 選択の frame | ユーザーが click、SSH で `select-frame ms=` を読む | 中央値 ≤ 50 ms、`SLOW-FRAME` 0 |

demo の image は WS075 の `plan/ws075/demo/build-demo-image.sh`（既定 `build/demo-hdmi`、`ZEDBSD_BOOT_EXTRA_LINES ?= display=edp`）。kei の autologin で session が始まり、
root に guest の harness の鍵（`plan/tmp/guest/id_ed25519`）が入る（同 script 41〜44 行）。

## 7. 注意

- 判定に QEMU の console・serial の log を使わない。push しない。commit は `git commit -m WIP -- <自分の path>`（AGENTS.md）。
- subagent が変えてよいのは WS094 の source（`userland/desktop/files/` の desktop の部分、p002 の範囲の compositor）と `plan/ws094/`。
  master・queue・guardrail・他の WS の plan は main に頼む。compositor（`userland/desktop/wayland/`）は WS099 の担当でもあるので main の許可を取る（design §9）。
- toolchain（`build/llvm` ほか）を変えない・build しない。`build-probe.sh` は `build/llvm/bin/clang` と `build/amd64/sysroot` を読むだけ。
- `build/amd64/` に作らない（他の WS の既定の image）。image の build は同時に 1 つ。
- runtime の directory の衝突: `build/ws094-run`（files-desktop-guest・desktop-guest の既定）、`build/ws094-p010-run`、`build/ws035-sq-run`（criteria.sh の固定、C9）。
  同じ runtime の guest を 2 つ動かさない。criteria.sh と並べない。
- ユーザーの判断: 溢れた icon は「今のまま」（描かない、2026-09-30）。L3 の合否は実機（Q1、2026-09-30）。p010 は統合の試験で合格（ユーザー、2026-09-30）。
- WS090 の p009・p010（Files の libkeiui への移行）と Files の同じ file を同時に変えない（design §5）。WS104 の p007 は `userland/desktop/files/` の path の文字列を
  macro にする（`plan/ws104/patches/p007.patch`）ので、p011 の後に WS104 p007 が来るなら patch の当たりを main が確かめる。
- IME（WS095）の source（`userland/desktop/ime/` と compositor の IME の hook）は人間が作業中で触らない（master 2026-09-30）。
