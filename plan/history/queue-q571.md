<!-- awesome-plan project=zedbsd record=queue -->

# Queue

Active Queue: なし
Last finished Queue: q571
Status: finished
Cycle: q571
Approval: current user /2026-10-02 JST /this chat「WS108の完了後、WS109の実行をお願いします。」＋「drm-kmodを利用OKです。FreeBSDにも例外を適用します。FreeBSD実機は用意しておくので、作業を進めておいてください。」。WS109の既存目標内で実装先行、実機検証はユーザーが免除、最新ユーザー指示でi915 PCI passthroughの実GPU検証を選択。
Timebox: 最大20分 /1Phase
Focus: fg016 /WS109、WIP commit/pushなし、GitHub publication deferred。
Snapshot: [approved Phase](history/ws109/q571/approved-phase.md)、SHA256 b0c3859530008d0e6412013f1186775818092df9e177ebccb43d2c644fc32b91

| Attempt | Phase | Exact scope | Status | Dependencies |
| --- | --- | --- | --- | --- |
| q571-i01 | [ws109p004](ws109/phase004/phase.md) | F4 closure: hash-stable actualOSS/nativewired/radioABI/WPAwire receipts plus current nativeguest identity; physicalWiFi userwaived. | cleared | verified p001 software/ABI/environment output; further scope-specific prerequisites in Phase |

Dependency graph: verified p001 software/ABI/environment output; further scope-specific prerequisites in Phase → q571-i01。実機検証は免除。実FreeBSD QEMU Venus出力は未確認であり、host能力だけではacceptanceでない。
Started UTC: 2026-10-02T01:25:53.520460+00:00

## Upcoming Work Outlook

本Queueの実outputから次の有限Phaseを選定。FreeBSD QEMU実GPU利用とC全文review/affected3OS回帰を確認。WS106の保留は別。

Outcome: q571-i01 cleared /whole Phase cleared。F4 verified using unchanged actualOSS/nativewired/radioABI/WPAwire evidence and current nativeguest; physicalradio userwaived, mock correctlyclassified. [result](/home/awe/zedBSD-claude1/plan/history/ws109/q571/result.md). F5mainapps/finalreview remains.
Finished UTC: 2026-10-02T01:26:11.776580+00:00
