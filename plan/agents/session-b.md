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

