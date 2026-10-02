<!-- awesome-plan project=zedbsd record=ws094-p011 -->

# ws094-p011: L4b 溢れた項目の数の log と、無い名前の保存の行の掃除

Status: cleared（2026-10-02、q582-i01 / Agent B2、host・warning0 build・guest・boot PASS）
Disposition: normal
Parent: [WS094](../ws.md)
Queue: q582 / q582-i01（最大3時間、2026-10-02 user「N=3で作業を開始してください」）
依存: p010（cleared）
実行者の目安: phase-runner-mid（Files の desktop の部分だけ。compositor は触らない）

## 範囲（ws.md の段 L4 の (c)）

- grid の cell より多い項目は描かない（ユーザーの判断「今のまま」2026-09-30。**描き方は変えない**）。描かなかった数を log に出す。
- `~/.config/keiland/desktop-layout` の、`~/Desktop` に無い名前の行は、次の保存で消える（design §4「無い名前の行は次の保存で消える」）。
  今は `fm_desktop_layout_set`（`userland/desktop/files/desktop-layout.c:437`）が `desk->saved` を全て書くので、無い名前の行が残る（2026-10-01 に code で確かめた）。

範囲の外: 溢れた項目の表示（縮小・scroll・「さらに N 個」）、compositor、Files の窓の側。

## 手順（2026-10-01 追記）

1. `userland/desktop/files/desktop-layout.c` に、listing の名前で保存の場所を絞る関数を足す（宣言は `files.h`）:
   `void fm_desktop_layout_prune(struct fm_desktop *desk, const char *const *names, size_t count);`
   - `desk->saved` から `names` に無い名前の要素を詰めて除き、`saved_count` を減らす。file は書かない（次の `fm_desktop_layout_set`・
     `fm_desktop_layout_rename` の書き込みで消える）。除いた数が 1 以上なら `fm_log("DESKTOP prune removed=%lu kept=%lu", …)`。
   - 照らすのは listing の名前の全て（溢れて cell の無い項目も含む）。`desk->shown` は溢れた項目を含まない（`fm_desktop_remember`、同 556 行）ので使わない。
2. `userland/desktop/files/ui-desktop.c` の配置の所（632 行の `fm_desktop_arrange(names, tab->listing.count, known, …)` の前）で、
   **listing の読み込みが成功した時だけ** `fm_desktop_layout_prune(desk, names, tab->listing.count)` を呼ぶ。listing が失敗した時（error の時）は呼ばない
   （全ての行を消さないため）。listing の成否の field 名は `ui-desktop.c` と `dir.c` を読んで確かめる。
3. 同じ file の 195 行の `DESKTOP ready items=%lu cells=%d width=%d height=%d at_ms=%llu newest_age_ms=%lld` の**末尾に** ` hidden=%lu`
   （items − cells、0 以上）を足す。既存の試験の pattern（`ready items=5 cells=5 width=1280 height=766`、`ready items=100 ` など）は前方一致なので壊れない。
   念のため `grep -rn "DESKTOP ready" plan/ws094/tests plan/tools plan/ws0*/tests` で行末に依る pattern が無いことを確かめる。
4. host の試験 `plan/ws094/tests/host-desktop.c` に `check_prune` を足す: saved に `a.txt`（有る）・`gone.txt`（無い）・`far.txt`（有るが grid の外）を置き、
   names = {a.txt, far.txt, new.txt} で prune → saved は a.txt と far.txt の 2 つ、`fm_desktop_layout_set` の後の file に `gone.txt` の行が無い。
5. guest の手順 `prune` を `plan/ws094/tests/files-desktop-guest.sh` に足す（`saved` の手順、187〜201 行を手本に）:
   1. compositor を止め、layout file に `notes.txt	2	3` と `ghost.txt	4	4`（`~/Desktop` に無い）を書く。
   2. compositor を `--desktop-client='/bin/files --desktop'` で起こし（`show` の手順の起動の行を写す）、`DESKTOP prune removed=1 kept=1` を待つ。
   3. notes.txt を drag で別の cell へ動かす（`drag` の手順の pointer の動き）。layout file に `ghost.txt` の行が無く、notes.txt の新しい行がある（`cat` で読む）。
   4. `DESKTOP ready … hidden=0` の行がある。
   - 先頭の注釈の手順の一覧（2〜33 行）に `prune` の 1 項目を足す。
6. build・style・試験（下の「完了の条件」の command）。

## 完了の条件

| 確認 | command（repo の root、`<W>` = `ws094-p011`） | PASS |
| --- | --- | --- |
| build | `make -j64 ZEDBSD_CONFIG=plan/ws102/tests/config-amd64-inset.mk BUILD=build/<W>-inset build/<W>-inset/bin/files build/<W>-inset/bin/wayland build/<W>-inset/dynamic/libkeiland.so > build/<W>/bin-build.log 2>&1; echo "make exit=$?"` | `make exit=0`、`grep -c 'warning:' build/<W>/bin-build.log` が 0 |
| style | `python3 plan/tools/style-check.py userland/desktop/files/desktop-layout.c userland/desktop/files/ui-desktop.c userland/desktop/files/files.h plan/ws094/tests/host-desktop.c`、`git diff --check` | 新しい違反 0 |
| host | `sh plan/ws094/tests/host-desktop.sh`、`sh plan/tools/files/host-model.sh` | `host-desktop: PASS`（check_prune を含む）、`files-model: PASS` |
| guest（新） | guide.md §5.3 の起動の後 `BIN=build/<W>-inset sh plan/ws094/tests/files-desktop-guest.sh build/<W>/prune install show saved prune` | `files-desktop-guest: PASS` |
| guest（回帰） | 同じ guest で `… build/<W>/l1 install show watch input saved`、`… build/<W>/drag install show drag` | どちらも PASS |
| boot | `OUTPUT=build/<W>/boot plan/tools/boot-test.sh build/<W>-inset/hdd-image.img` | `boot-test: PASS`、PNG をユーザーに見せる |

image は guide.md §5.1 の `build-inset-image.sh build/<W>-inset` で 1 回作る（Phase の最初）。build/amd64 は使わない。

## q582 checkpoint（2026-10-02）

[結果と再開点](q582-result.md)。hidden/pruneと専用host/guest試験を実装。host3本、warning0 target build、prune/L1/drag/menu guest回帰、最終staged imageのboot-testがPASS。Phase/attemptはcleared。fresh full-image生成はNoct/toolchain制限のため実施せず、B mainが指定した検証済みfixtureの独立コピーへ現行Files/libraryを入れた。詳細・hash・制限は結果記録に保持。
