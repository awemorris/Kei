# Temporary A/B coordination / 2026-10-02

Agent A checkout: `/home/awe/zedBSD-claude1`, current branch `codex/agent-a` (user/environment branch choice; replaces startup document's main branch assumption).
Agent B checkout: `/home/awe/zedBSD-claude2`, independently owned by B.

For this first N=3 cycle, reserve q581–q583 for B and q584–q586 for A. B's local records remain B-owned drafts; A does not mutate B's checkout. Subsequent numeric IDs are allocated by A after read-back of both lane records. If B already allocated different IDs, preserve both records and reconcile; never renumber history cosmetically.

A active scope: q584/WS074 p172 final review, q585/WS112 p001 contract research, q586/WS113 p001 display design research. A2/A3 edit only their WS documentation and evidence. A1 edits browser/libbrowser and WS074 source/test evidence. B owns its assigned desktop/GTK/bug work; no A runtime claims shared hardware.

No next Queue is automatically authorized. WIP commits, no push. Shared plans/GitHub remain A-owned; B projection changes are handed to A in commits for review/integration.
