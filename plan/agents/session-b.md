# Agent B startup

Temporary owner: Agent B  
Checkout: `/home/awe/zedBSD-claude2`  
Branch: `codex/agent-b`, based on the current `origin/main`  
Subagent capacity: three concurrent children

Read `AGENTS.md`, the Guardrail, `plan/master.md`, current Queue, latest Past
Log and the relevant WS/Phase before selecting work.  Assignment is not Queue
authorization.  Agent A owns the shared Master/Queue/history/Guardrail/cache and
GitHub sync.  B owns its WS/Phase records, B lane Queues, worktrees and evidence;
send shared projection changes to A as a commit/checkpoint instead of editing
the same shared records concurrently.

## Three lanes

1. GTK/Qt: WS114, WS115, WS116, WS097 and WS096.  Follow the required order:
   Linux GTK4 evidence, user row-by-row decision, selected compositor/portal
   work, upstream GTK4, Qt6, then independent implementations.
2. Keiland desktop/UI: WS078, WS079, WS081, WS085, WS089, WS090, WS094,
   WS099, WS100, WS102 and WS110.  WS110 remains design-only until authorized.
   Do not edit WS113 or its Settings/display work, which belongs to A.
3. Bugs: WS073 and handling WS/Phases selected from the Bug Board.  Resume
   BUG-125 from q577 evidence first when a finite Queue is selected.  Browser
   source bugs return to A; a ticket alone does not authorize a fix.

Create a finite Phase Queue before implementation.  Use isolated worktrees for
the three children, commit with `git commit -m WIP`, and do not push unless the
user authorizes that push.  Never edit an A-owned WS concurrently.

Initial setup is complete only after `main` has been fast-forwarded from
`origin/main`, `codex/agent-b` points at that commit, and `git status --short`
is empty.  Record the exact handoff SHA in the first B checkpoint.


## Continuity instruction / 2026-10-02

Current user:「サブエージェントのコンテキスト読み込みが繰り返されるのは、トークンが無駄になります。サブエージェントは終了せずに、次々とqueueを送り込んで、長時間稼働させてください。」

B maintains the same B1/B2/B3 agent identities, worktrees and loaded context.
Queue completion produces an MR/checkpoint and dispatch waiting state rather
than voluntary wrap-up. Main prepares and dispatches subsequent finite Queues
within the assigned WS/bug scope, with A-assigned global IDs, verified dependencies
and resource ownership. Existing rules need only changed sections and the next
scope's missing material reloaded. User wrap-up/stop still takes effect promptly.
No context reload or agent replacement is required solely because a Queue ends;
actual runtime/context limits are reported accurately when observed.

## Wrap-up instruction / 2026-10-02

Current user:「B1,B2に現在のphaseを完了したらラップアップして終了するようにお伝えください。B3に作業状況を説明するようにお伝えください。」

This supersedes continuous successor dispatch for B1 and B2. B1 finishes the
current WS114 p007/q587 attempt within its approved bounds, records outcomes and
cleanup, then ends. B2 finishes the current scoped WS094 p007/q588 attempt,
preserves whole-Phase residual obligations, records outcomes and cleanup, then
ends. Neither lane starts a successor Queue. Reserved q593/WS099 p019 remains
unstarted with its authorized wallpaper decisions and design preserved for a
later resume; this is not Phase cancellation. B3 was asked for a status report;
its existing finite q589 scope/resource wait remains unchanged.

## B3 wrap-up instruction / 2026-10-02

Current user:「了解、B3には作業状況を保存してもらい、unclearedで記録してもらい、再開できるようにした上で、終了してもらってください。」

B3 must not start the waiting q589 guest runs. Save preparation, verification,
remaining work and resume requirements; end q589-i01 uncleared and wrap up.
Whole WS099 p017 remains uncleared and BUG-125 remains tracking. Preserve all
earlier attempt outcomes and prepared artifacts. Main verifies the terminal
submission and maintains pending A-owned shared history/projections. This latest
direction ends B3 as well as B1/B2 after their respective wrap-ups; no successor
Queues are dispatched.
