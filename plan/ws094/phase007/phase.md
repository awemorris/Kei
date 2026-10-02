<!-- awesome-plan project=zedbsd record=ws094-p007 -->

# ws094-p007: 全文の規約と回帰（WS094 の最後）

Status: planned（2026-10-01 に phase.md を作った。手順は下）
Disposition: normal
Parent: [WS094](../ws.md)
Queue: q588-i01（source/host/build partial、全guestと実機gateは保持）
依存: main が選んだ最後の段の Phase（今の見込み: p011、p012。ws.md「段」と guide.md §3）
実行者の目安: phase-runner（Files と compositor の両方を見る）

## 範囲と受け入れ

Awesome Plan §6 の「code を作る WS の最後の conformance の Phase」。WS094 が変えた全ての source を [plan/coding-style.md](../../coding-style.md) の全文で見直し、
format・style・build・試験を流し、範囲の中の違反を直す。新しい機能は足さない。

## 手順（2026-10-01 追記）

1. WS094 が変えた file の一覧を作る（commit の範囲は ws094 の Phase の記録の commit と `git log`）。
   ```
   mkdir -p build/ws094-p007
   git grep -l -E "fm_desktop|keiland_desktop|DESKTOP (ready|place|select-frame)|desktop-layout" -- userland include > build/ws094-p007/files.txt
   cat build/ws094-p007/files.txt
   ```
   少なくとも次を含む: `userland/desktop/files/{ui-desktop.c,ui-desktop-drag.c,ui-desktop-actions.c,desktop-layout.c,files.h,main.c,present.c,window.h,thumb.c,Makefile}`、
   `userland/desktop/picture/{picture.c,picture.h}`、`userland/desktop/wayland/desktop.c` と p002 で足した compositor の部分、`userland/desktop/libkeiland/desktop.c`、
   `include/libc/keiland.h` の desktop の節、`plan/ws094/tests/*.c`。
2. style の機械の確かめ:
   ```
   python3 plan/tools/style-check.py $(cat build/ws094-p007/files.txt | grep -E '\.(c|h)$') plan/ws094/tests/*.c > build/ws094-p007/style.txt; echo "exit=$?"
   git diff --check
   ```
   既知の例外（直さないで記録する）: `files/main.c` の既存の 2 件（p008〜p009 の記録、187・573 行付近）、`picture.c` の `setjmp` の 1 件（p013、setjmp は条件の中に置く必要がある）、
   `thumb.c` の `plan/tools/imageview/style-extra.py` の既存の 3 件。行番号は変わっているので中身で照らす。
3. 全文の規約の目視（機械で拾えない規則: 注釈の文体、関数の大きさ、名前、error の返し方）を、手順 1 の file の WS094 の部分について行い、直した点を表に記録する。
4. build（warning 0）と試験の全て（[guide.md](../guide.md) §5）:
   ```
   sh plan/ws102/tests/build-inset-image.sh build/ws094-p007-inset > build/ws094-p007/inset-build.log 2>&1; echo "exit=$?"
   grep -E ':[0-9]+:[0-9]+: warning:' build/ws094-p007/inset-build.log | grep -vE '/packages/|^\.\./src/|userland/base/noct/noct/' | wc -l
   sh plan/ws094/tests/host-desktop.sh; sh plan/ws094/tests/host-thumb.sh build/ws094-p007/host-thumb; sh plan/tools/files/host-model.sh; sh plan/tools/imageview/run-host.sh
   sh plan/ws094/tests/build-probe.sh build/ws094-p007-inset
   ```
   guest（guide.md §5.3、`<W>` = `ws094-p007`）: `files-desktop-guest.sh` の `install show watch input saved`・`install show menu`・`install show drag`・
   `install show touch`・`install perf100`（値は参考）・p011 があれば `install show saved prune`、`desktop-guest.sh … install role refuse input home-drag dnd restart` を **2 回**
   （BUG-123 の後で restart が通ることの確かめ）、`desktop-p010.sh`。
   Files の guest 試験 14 本（guide.md §5.4 の files-regress）。WS099 の C9（`plan/ws104/commands.md` §5 の command で `C9` だけ、`build/ws094-p007-criteria`）。
   boot test: `OUTPUT=build/ws094-p007/boot plan/tools/boot-test.sh build/ws094-p007-inset/hdd-image.img`。
5. 記録: 範囲、command、版（commit）、結果、例外、流さなかった試験とその理由、制限。QEMU と実機を分けて書く（実機は p012）。

## 完了の条件

- 手順 2 の style が既知の例外のほか 0、`git diff --check` 0、build の warning 0。
- host 4 本 PASS、guest の `files-desktop-guest: PASS`（全ての手順）、`desktop-guest: PASS`（2 回とも）、`desktop-p010: PASS`、`files-regress: PASS`、
  C9 の results.txt が全て PASS、`boot-test: PASS`（PNG をユーザーに見せる）。
- cleared の後、main が WS094 を完了の形に書き直す（guide.md §1.2）。

2026-10-02 / b2-q588-partial-dispatch: ユーザー連続Queue指示により[q588 exact partial scope](q588-approved-scope.md)を投入。p011実出力clear後にsource inventory/全文reviewとhost/buildを先行。B1所有waylandは読取reviewのみ。whole p007の実機/最終guest条件は保持、partial item clearanceと区別する。
