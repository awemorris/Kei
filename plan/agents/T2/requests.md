# T2 の試験の依頼（受付の台帳）

[protocol](../protocol.md) の「試験の担当 T1」と同じ運用。T1 から移管された依頼と、新しく T2 に来た依頼を記録する。ID は T2-001 から。

| ID | 依頼主 | 対象 | 状態 |
| --- | --- | --- | --- |
| T2-001 | P1（ws014-p011、BUG-144・124 の案 1、HAL の実装の変更）。Q1 が移管分より先に指示 | agent/p1 ac703b04b（amd64 の device の窓を 2 GiB へ、Venus の上限 1 GiB、hostmem の既定 1G）。1) その commit の image で `plan/tools/boot-test.sh`（Venus 無しの普通の起動）。2) CI の image（config/ci/config-amd64.mk）+ `files-guest.sh start` で `aperture-bug144.sh OUT/bug144-1g`、時間があれば `VENUS_HOSTMEM=256M` で `OUT/bug144-256m`。返す物: mview-steps.txt の何個目で失敗、`aperture full` の aperture・used・largest_hole。起動しない・session が出ないなら guest の `dmesg \| grep venus` | 予約 |
| T2-002 | P2（依頼 2、q645 / ws099-p026、BUG-147）。T1-028 から移管 | agent/p2 ebe770901（9e29a90bf を含む）の criteria の image（`build-criteria-image.sh`）で `C9_TESTS="p128 plan/ws099/tests/cursor-owner.sh" criteria.sh … C9`。PASS: results.txt の 2 行とも PASS、p128.start.log・cursor-owner.start.log に `guest: ready`、`guest start 1 failed` 無し（あれば行を返す） | 予約 |
| T2-003 | P2（依頼 3、q645 / ws099-p027、BUG-095 の init）。T1-033 から移管 | 1) P2 の image A（`p2/build/p2-bug095-img`、期待 `bug095: PASS`）・B（`p2/build/p2-bug095-old`、期待 `bug095: FAIL`）の複写で `bug095-test.sh <image> 180`。2) T2-002 の criteria の image で boot-test.sh と `C1_REQUIRE_QEMU_EXIT=1 c1-boot-shutdown.sh` が `C1-boot: PASS` | 予約 |
| T2-004 | P2（q645 / ws099-p028、BUG-142・BUG-141、compositor の入力） | agent/p2 2217907f6 の `build-files-image.sh` の image（IME 無し）+ `files-guest.sh start` で bug142-order.sh・bug141-hover.sh（PNG base・hover-on・menu）、回帰 cursor-owner.sh・zdesktop-p077.sh・menu-p003.sh・files-p004.sh・zdesktop-p084.sh が各 PASS。任意で 25cfd28e1 以前の compositor の対照 | 予約 |
| T2-005 | P1（依頼 13、BUG-099 の再現の試み）。T1-029 から移管 | agent/p1 c5a56c717 の WS081 の pen の image で `plan/ws081/tests/p012-guest.sh BUILD OUT/p012-N` を 3 回まで、`touchinject swipe.script: FAILED` の有無（出たら `*.failed.log`） | 予約（最後） |
