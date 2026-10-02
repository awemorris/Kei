<!-- awesome-plan project=zedbsd record=queue -->

# Queue

Active Queue: なし
Last finished Queue: q568
Status: finished
Cycle: q568
Approval: current user /2026-10-02 JST /this chat「WS108の完了後、WS109の実行をお願いします。」＋「drm-kmodを利用OKです。FreeBSDにも例外を適用します。FreeBSD実機は用意しておくので、作業を進めておいてください。」。WS109の既存目標内で実装先行、実機検証はユーザーが免除、FreeBSD QEMU Venus実利用が受け入れ条件。
Timebox: 最大45分 /1Phase
Focus: fg016 /WS109、WIP commit/pushなし、GitHub publication deferred。
Snapshot: [approved Phase](history/ws109/q568/approved-phase.md)、SHA256 8ac31fd88b8dbf474e48e8c34ede457cd7afb3af75d4d948d1c029343c026a33

| Attempt | Phase | Exact scope | Status | Dependencies |
| --- | --- | --- | --- | --- |
| q568-i01 | [ws109p003](ws109/phase003/phase.md) | p003 partial: awe@10.0.10.25 owned FreeBSD guest, existing vfio IrisXe passthrough, native i915/DRM/Vulkan capability and command verification. | uncleared | verified p001 software/ABI/environment output; further scope-specific prerequisites in Phase |

Dependency graph: verified p001 software/ABI/environment output; further scope-specific prerequisites in Phase → q568-i01。実機検証は免除。実FreeBSD QEMU Venus出力は未確認であり、host能力だけではacceptanceでない。
Started UTC: 2026-10-02T00:28:04.080722+00:00

## Upcoming Work Outlook

本Queueの実outputから次の有限Phaseを選定。FreeBSD QEMU Venus実利用とC全文review/affected3OS回帰を確認。WS106の保留は別。

Outcome: q568-i01 uncleared /whole Phase uncleared。Native i915/Intel Vulkan1MiB/offscreen and realunpriv compositor-shm PASS; liveDMA_BUF zeroaccessflags block ioctl/Vulkanwindow. [result](/home/awe/zedBSD-claude1/plan/history/ws109/q568/result.md) / [BUG-130](/home/awe/zedBSD-claude1/plan/bugs/BUG-130.md). Nextbounded native capability adaptation; no driverpatch/falseclear. Own remoteVMrunning, baselineVFIOretained.
Finished UTC: 2026-10-02T01:05:38.625437+00:00
