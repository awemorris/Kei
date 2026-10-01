<!-- awesome-plan project=zedbsd record=queue -->

# Queue

Active Queue: なし
Last finished Queue: q561
Status: finished
Cycle: q561
Approval: current user /2026-10-02 JST /this chat「WS108の完了後、WS109の実行をお願いします。」＋「drm-kmodを利用OKです。FreeBSDにも例外を適用します。FreeBSD実機は用意しておくので、作業を進めておいてください。」。WS109の既存目標内で実装先行、実機関門を保持。
Timebox: 最大60分 /1Phase
Focus: fg016 /WS109、WIP commit/pushなし、GitHub publication deferred。
Snapshot: [approved Phase](history/ws109/q561/approved-phase.md)、SHA256 f48fc61d4ee9522dade3a5fe56768cd9d7f15fafbb677545f76dafd75ac7e6aa

| Attempt | Phase | Exact scope | Status | Dependencies |
| --- | --- | --- | --- | --- |
| q561-i01 | [ws109p002](ws109/phase002/phase.md) | p002 partial: native PTY header/libutil and real extattr adapters・UFS/PTY contract tests; common app source unchanged/fullapps/F2 retained。 | cleared | verified p001 software/ABI/environment output; further scope-specific prerequisites in Phase |

Dependency graph: verified p001 software/ABI/environment output; further scope-specific prerequisites in Phase → q561-i01。外部の実機準備はcontext、native実装先行はユーザー承認。未着実機はverified acceptanceでない。
Started UTC: 2026-10-01T23:20:15.667719+00:00

## Upcoming Work Outlook

本Queueの実outputから次の有限Phaseを選定。GPU/WiFiの実機関門、C全文review/affected3OS回帰を保持。WS106の保留は別。

Outcome: q561-i01 cleared /whole Phase uncleared。native PTY header/libutil・extattr adapter、実UFSのfd/path/link/コピー/権限/ERANGEと実PTY child入出力PASS。[result](/home/awe/zedBSD-claude1/plan/history/ws109/q561/result.md)。fullapps/nativeGUI/F2・threeOS/p005/physical gatesは保持。
Finished UTC: 2026-10-01T23:30:23.237120+00:00
