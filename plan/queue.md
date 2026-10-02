<!-- awesome-plan project=zedbsd record=queue -->

# Queue

Active Queue: なし
Last finished Queue: q572
Status: finished
Cycle: q572
Approval: current user /2026-10-02 JST /this chat「WS108の完了後、WS109の実行をお願いします。」＋「drm-kmodを利用OKです。FreeBSDにも例外を適用します。FreeBSD実機は用意しておくので、作業を進めておいてください。」。WS109の既存目標内で実装先行、実機検証はユーザーが免除、最新ユーザー指示でi915 PCI passthroughの実GPU検証を選択。
Timebox: 最大60分 /1Phase
Focus: fg016 /WS109、WIP commit/pushなし、GitHub publication deferred。
Snapshot: [approved Phase](history/ws109/q572/approved-phase.md)、SHA256 643941dce9a4f004e6ff08550e1b100261813ff8b9d12332eb0aabbe9208e8d4

| Attempt | Phase | Exact scope | Status | Dependencies |
| --- | --- | --- | --- | --- |
| q572-i01 | [ws109p005](history/ws109/q572/phase.md) | F5 actualnativeIntelGPU mainapp operations, addedsource fullstandard/nativebuild/header/ELF/docs; ownWSacceptance/recordcleanup ifallcriteria satisfied. | cleared | p002/q565、p003/q570、p004/q571 cleared outputs +q566 final-review/regression receipts |

Dependency graph: p002/q565、p003/q570、p004/q571 cleared outputs +q566 final-review/regression receipts → q572-i01。実機検証は免除。実FreeBSD QEMU Venus出力は未確認であり、host能力だけではacceptanceでない。
Started UTC: 2026-10-02T01:27:14.547590+00:00

## Upcoming Work Outlook

WS109 completed、fg016達成。新Queue未選定。既存demo WSの優先とWS106の保留を維持。

Outcome: q572-i01 cleared /whole Phase cleared。Actual sevennativeGPU mainapps/PTY/fileopens plus AppHome PASS; fullsource C fixes/review/nativeheaderELF/3OS builds/finalboot/docs verified. [result](/home/awe/zedBSD-claude1/plan/history/ws109/q572/result.md). F1..F5 satisfied under userselectedi915/physicalWiFiwaiver; BUG130 upstreamtracking, ownVMstopped/VFIOpreserved.
Finished UTC: 2026-10-02T01:47:37.313116+00:00

WS acceptance: WS109 F1〜F5 verified/completed、actualFreeBSD QEMU i915 route。BUG130 upstreamtracking、Venus未実行、physicalradio userwaived。専用VM停止/VFIO復元状態、WIP/no push/publicationdeferred。
