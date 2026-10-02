<!-- awesome-plan project=zedbsd record=q577 -->

# q577: BUG-125 move/resize同期診断

Status: finished
Attempt: q577-i01 / uncleared
Phase: [ws099-p017](../ws099/phase017/phase.md)
Approval: current user / 2026-10-02「N=3でしばらく実行」
Executor: P8 generation1/2、canonical writer Q1

BUG-125の再現と試験同期を3時間の有限範囲で診断した。accepted request countを待つ試験修正は保存したが、whole p076は16完了中15 PASS/1 FAIL、完全C9は1回10/10 PASS、明示rendererの補足は途中5 PASS/1 FAILで、zero-failure/所定回数を満たさない。compositor sourceはread-onlyで、BUG-125はtracking、Phaseはuncleared。

[Terminal result](../ws099/phase017/q577-result.md)に原failure、補足、環境誤り、p128別症状、conformance、cleanup、再開資産を保存した。P8 commit `23065bf05`をmain `c1487ae3f`へ統合。owned QEMU/renderer/diagnostic processとlistenerは停止済み。再開には症状を分離した新しい有限Queueが必要。
