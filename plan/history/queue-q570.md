<!-- awesome-plan project=zedbsd record=queue -->

# Queue

Active Queue: なし
Last finished Queue: q570
Status: finished
Cycle: q570
Approval: current user /2026-10-02 JST /this chat「WS108の完了後、WS109の実行をお願いします。」＋「drm-kmodを利用OKです。FreeBSDにも例外を適用します。FreeBSD実機は用意しておくので、作業を進めておいてください。」。WS109の既存目標内で実装先行、実機検証はユーザーが免除、最新ユーザー指示でi915 PCI passthroughの実GPU検証を選択。
Timebox: 最大35分 /1Phase
Focus: fg016 /WS109、WIP commit/pushなし、GitHub publication deferred。
Snapshot: [approved Phase](history/ws109/q570/approved-phase.md)、SHA256 bb800d1bea0359676a58738b9aa3ee8114e6dd2fad3e230e115fc92ff3e55e92

| Attempt | Phase | Exact scope | Status | Dependencies |
| --- | --- | --- | --- | --- |
| q570-i01 | [ws109p003](ws109/phase003/phase.md) | p003 partial: actualGPU VT ownership/notification diagnosis and bounded native integration correction; real pause/resume/input/cleanup. | cleared | verified p001 software/ABI/environment output; further scope-specific prerequisites in Phase |

Dependency graph: verified p001 software/ABI/environment output; further scope-specific prerequisites in Phase → q570-i01。実機検証は免除。実FreeBSD QEMU Venus出力は未確認であり、host能力だけではacceptanceでない。
Started UTC: 2026-10-02T01:21:38.551352+00:00

## Upcoming Work Outlook

本Queueの実outputから次の有限Phaseを選定。FreeBSD QEMU実GPU利用とC全文review/affected3OS回帰を確認。WS106の保留は別。

Outcome: q570-i01 cleared /whole Phase cleared。Actual nativeGPU window/VT asynchronous lease retirement→reacquisition/input/93frames/cleanup PASS; q569 DMAownership/window output retained. [result](/home/awe/zedBSD-claude1/plan/history/ws109/q570/result.md). Whole F3 cleared, F4closure/F5mainapps/finalstandards remain.
Finished UTC: 2026-10-02T01:24:59.323827+00:00
