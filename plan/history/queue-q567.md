<!-- awesome-plan project=zedbsd record=queue -->

# Queue

Active Queue: なし
Last finished Queue: q567
Status: finished
Cycle: q567
Approval: current user /2026-10-02 JST /this chat「WS108の完了後、WS109の実行をお願いします。」＋「drm-kmodを利用OKです。FreeBSDにも例外を適用します。FreeBSD実機は用意しておくので、作業を進めておいてください。」。WS109の既存目標内で実装先行、実機検証はユーザーが免除、FreeBSD QEMU Venus実利用が受け入れ条件。
Timebox: 最大45分 /1Phase
Focus: fg016 /WS109、WIP commit/pushなし、GitHub publication deferred。
Snapshot: [approved Phase](history/ws109/q567/approved-phase.md)、SHA256 ad1832f41f06b3f009ac2a586b0dc7262a7e965533ec4f418e8e50dfc54ca17d

| Attempt | Phase | Exact scope | Status | Dependencies |
| --- | --- | --- | --- | --- |
| q567-i01 | [ws109p003](ws109/phase003/phase.md) | p003 partial: actual FreeBSD QEMU Venus host/kernel/Mesa capability chain, owned guest config/probes; no new kernel driver port. | uncleared | verified p001 software/ABI/environment output; further scope-specific prerequisites in Phase |

Dependency graph: verified p001 software/ABI/environment output; further scope-specific prerequisites in Phase → q567-i01。実機検証は免除。実FreeBSD QEMU Venus出力は未確認であり、host能力だけではacceptanceでない。
Started UTC: 2026-10-02T00:23:01.810188+00:00

## Upcoming Work Outlook

本Queueの実outputから次の有限Phaseを選定。FreeBSD QEMU Venus実利用とC全文review/affected3OS回帰を確認。WS106の保留は別。

Outcome: q567-i01 uncleared /whole Phase uncleared。Actual Venus-configured FreeBSD QEMU boots, but native DRM/Venus ICD absent and latest upstream virtio driver lacks HOST_VISIBLE. [result](/home/awe/zedBSD-claude1/plan/history/ws109/q567/result.md). Kernel/driver port outside WS109; concrete acceptance/scope decision requested, physical tests waived.
Finished UTC: 2026-10-02T00:26:10.545700+00:00
