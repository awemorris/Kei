<!-- awesome-plan project=zedbsd record=ws046-p015 -->
# ws046-p015: BUG-027 — file に裏付けられた page の fault の再計測（直っている見込みの確認と close）

Status: cleared（Q1 判定 2026-10-03: T1-034（QEMU、単独）cold 76.54 µs/page（基準 ≤180）、warm 1.71 µs/page（≤34）、warm の fault と read の比 1.0x。ticket の 1.8 ms/page から 20 倍以上改善）。元の記載: in-progress（q645、P2、2026-10-03。計測の道具と image を用意、T1 の計測待ち）
Disposition: normal
Parent: [WS046](../ws.md)
Bug: [BUG-027](../../bugs/BUG-027.md)

## 範囲（Q1、2026-10-03 user「実機なしで解決できるバグをどんどん処理してください」）

BUG-027（ticket の計測: libLLVM の先頭 32 MiB の `MAP_PRIVATE` の読みの fault が cold 1.8 ms/page、warm 0.34 ms/page、`read()` の 7〜36 倍）を
今の kernel で計り直し、直っていれば close を提案する。2026-09-28・10-03 の user: 低い優先度、直っている見込みで計測して閉じる。

## これまでの直し（記録の読み）

- ws046-p007・p009（BUG-033）: libc の allocator、buffer cache の hash、region の page の探索。kbench の file fault 22 → 18 µs。
- ws046-p012・p014: object の寿命（cache 化）と、private の file の mapping で page cache の page を直接 map（`VM_REGION_PRIVATE_OBJECT`）。
  2026-09-25 の QEMU で kbench の file fault 3.2 µs、`ffault libLLVM` 3.0〜3.5 µs/page、`clang --version` 60〜67 ms（ticket は 11〜15 s）。
- 今の `vmspace.c` は fault-around（`VM_FAULT_AROUND_PAGES` 16、cache にある隣を map）。cold の page の読みは 1 page ずつ同期
  （`vm.c` の fault の fill は `FILE_IO_VM_OBJECT` で、`file.c` の readahead の観測の対象外）。cold が遅ければ、ここが次の候補。

## 計測の道具（このPhase で作った）

- `plan/tools/kbench/ffault.c`: ticket の計測の再現（先頭 N MiB を `MAP_PRIVATE` で map して 1 page 1 byte 読む、同じ mapping の再読み、
  別の private の mapping への書き込み（COW）、同じ範囲の `pread` 1 MiB ずつ）。`plan/tools/kbench/build.sh BUILD OUTPUT ffault` で作る
  （build.sh に PROGRAM の引数を足した。既定は kbench）。ffault の元の program（p010〜p014 の `ffault`）は tree に無かったので作り直した。
- `plan/ws046/tests/bug027/build-bug027-image.sh BUILD`: SSH の guest の image に `/bin/ffault`・`/bin/kbench` と zedBSD の
  `libLLVM.so.23.1`（main の clang の package の stage から読み取りの複写、data として `/var/bug027/`）。toolchain は build しない。
- `plan/ws046/tests/bug027/bug027-test.sh IMAGE [OUTDIR]`: 新しく起こした guest で ffault 32 MiB を 2 回（cold・warm）と kbench。
  PASS: cold の読みの fault ≤ 180 µs/page（ticket の 1/10）、warm ≤ 34 µs/page（同）、warm の fault が `read()` の 10 倍以内。

## 確かめ

- host（Linux）: ffault を gcc で作って libLLVM で走らせた（read fault 0.8 µs/page、read() 1.1 µs/page）。判定の python を良い値・ticket の値で試し、
  PASS・FAIL を期待どおり返した。`style-check.py` は ffault.c に違反 0。guest 用の build（zedBSD の clang、-Werror）は warning 0。
- QEMU（T1 に依頼）: 結果は未着。

## 結果（T1-034、2026-10-03、QEMU の amd64 guest、他の QEMU の無い時に単独）

`bug027-test.sh` PASS（[T1 の出力](/home/awe/zedBSD-worktrees/t1/build/t1-bug027/t1-034/)）: libLLVM の先頭 32 MiB の MAP_PRIVATE の読みの fault が
cold 76.54 µs/page（ticket 1.8 ms の約 1/24）、warm 1.71 µs/page（ticket 0.34 ms の約 1/200）、warm の fault と `read()` の比 1.0 倍（ticket 7〜36 倍）。
kbench: file page fault 950 ns、anon page fault 2380 ns。BUG-027 は直っている（ws046-p007〜p014 の直し）。resolved を提案。cold は今も 1 page ずつ同期だが、
基準（180 µs）の内なので直さない。実機は未実施。
