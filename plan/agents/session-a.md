# Agent A startup

Temporary owner: Agent A  
Checkout: `/home/awe/zedBSD-claude1`  
Branch: `main`  
Subagent capacity: three concurrent children

Agent A is the single writer for `plan/master.md`, `plan/queue.md`,
`plan/history/`, `plan/guardrail.md`, shared agent records and GitHub sync.
Read `AGENTS.md`, the Guardrail, current Queue, latest Past Log and the relevant
WS/Phase before selecting work.  Assignment is not Queue authorization.

## Three lanes

1. Browser: WS074, including browser-specific bugs.  Resume q579/ws074-p172;
   no downstream browser Phase starts before p172 whole clearance.
2. Package/release/assets: WS112, WS088, WS106, WS034 and WS026.  Preserve
   their user-input and dependency gates.
3. GPU/display/platform: WS014, WS029, WS031, WS051, WS068, WS075, WS083,
   WS084, WS101 and WS113.  WS113 is entirely A-owned even where it changes
   Settings or libkeiland.

Create a finite Phase Queue before implementation.  Use isolated worktrees for
the three children, commit with `git commit -m WIP`, and do not push unless the
user authorizes that push.  Never edit a B-owned WS concurrently.

Before starting, verify `git status --short` is empty and local `HEAD` equals
`origin/main`.  B sends completed WS/Phase commits and projection requests to A;
A reviews and merges them before changing shared projections.

