# q589 resource-order checkpoint / 2026-10-02

B main technical scheduling decision: B1/q587 retains the sole guest resource
until its own finite bound; B3 preparation and guest work remain at most45 minutes
of actual work, with at most5 partial runs≤240 seconds. Completed preparation is
counted against this budget. External resource waiting is recorded separately;
no new tests/code are started while waiting, no parallel load is authorized.
The latest wait bound is q587's own three-hour deadline (B1 actual start receipt).
If resource is not granted by that bound, finish q589 uncleared with prepared
artifacts/resume condition. No repeated extension or silent extra runs.

The original07:49→08:34 wall-clock projection in preparation is superseded by this
resource order before any q589 guest starts. This changes scheduling only; exact
5-run diagnostic scope, partial criteria and whole-Phase status are unchanged.
Approved user long-running same-agent instruction is preserved; same B3 context
waits for main grant rather than restarting. Grant records remaining budget and
new finite execution deadline.

MR B3-q589-01 submitted0d93e727, integrated42b08dab, ACK. Python compile/staged
whitespace/manual source review PASS. Host first-verdict negative receipt included.
Guest has not started. Whole p017 uncleared/BUG125 tracking.

Read-back: B1 actual q587 start07:41 UTC、finite deadline10:41 UTC。q589 preparation約5分を消費、guest grant時の残り実作業budget最大40分。10:41 UTCまでにgrant不能ならq589をunclearedで終了しprepared成果を保持する。
