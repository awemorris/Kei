<!-- awesome-plan project=zedbsd record=queue -->

# Queue

Active Queue: なし
Last finished Queue: q569
Status: finished
Cycle: q569
Approval: current user /2026-10-02 JST /this chat「WS108の完了後、WS109の実行をお願いします。」＋「drm-kmodを利用OKです。FreeBSDにも例外を適用します。FreeBSD実機は用意しておくので、作業を進めておいてください。」。WS109の既存目標内で実装先行、実機検証はユーザーが免除、最新ユーザー指示でi915 PCI passthroughの実GPU検証を選択。
Timebox: 最大40分 /1Phase
Focus: fg016 /WS109、WIP commit/pushなし、GitHub publication deferred。
Snapshot: [approved Phase](history/ws109/q569/approved-phase.md)、SHA256 a27121a43bd632149003610865ec9b5d45f9322660417f418959ffda16bf21a3

| Attempt | Phase | Exact scope | Status | Dependencies |
| --- | --- | --- | --- | --- |
| q569-i01 | [ws109p003](ws109/phase003/phase.md) | p003 partial: native-only zeroaccess DMA-BUF transport classification to existing CPUcompletion fallback; realGPU window/ownership/native error tests. | cleared | verified p001 software/ABI/environment output; further scope-specific prerequisites in Phase |

Dependency graph: verified p001 software/ABI/environment output; further scope-specific prerequisites in Phase → q569-i01。実機検証は免除。実FreeBSD QEMU Venus出力は未確認であり、host能力だけではacceptanceでない。
Started UTC: 2026-10-02T01:07:15.368956+00:00

## Upcoming Work Outlook

本Queueの実outputから次の有限Phaseを選定。FreeBSD QEMU実GPU利用とC全文review/affected3OS回帰を確認。WS106の保留は別。

Outcome: q569-i01 cleared /whole Phase uncleared。Native zeroaccess capability adaptation and actual mappedVulkan3frames/ownership PASS; realGPU VT notification/lease test failed, remains F3. [result](/home/awe/zedBSD-claude1/plan/history/ws109/q569/result.md). Next finite nativeVT integration investigation, no falseclear/kernelrepair.
Finished UTC: 2026-10-02T01:21:16.716541+00:00
