# Temporary A/B coordination / 2026-10-02

Agent A checkout: `/home/awe/zedBSD-claude1`, current branch `codex/agent-a` (user/environment branch choice; replaces startup document's main branch assumption).
Agent B checkout: `/home/awe/zedBSD-claude2`, independently owned by B.

For this first N=3 cycle, reserve q581–q583 for B and q584–q586 for A. B's local records remain B-owned drafts; A does not mutate B's checkout. Subsequent numeric IDs are allocated by A after read-back of both lane records. If B already allocated different IDs, preserve both records and reconcile; never renumber history cosmetically.

A active scope: q584/WS074 p172 final review, q585/WS112 p001 contract research, q586/WS113 p001 display design research. A2/A3 edit only their WS documentation and evidence. A1 edits browser/libbrowser and WS074 source/test evidence. B owns its assigned desktop/GTK/bug work; no A runtime claims shared hardware.

No next Queue is automatically authorized. WIP commits, no push. Shared plans/GitHub remain A-owned; B projection changes are handed to A in commits for review/integration.

Each `approved-phase.md` is a byte-preserved approval snapshot. Its relative links retain the original Phase directory as their base, specified by the `Phase:` link in the accompanying lane Queue; resolve links there instead of rewriting the approved bytes. The current Phase and lane are the navigable records. All six snapshot hashes were checked on A after B record import.

2026-10-02 / B-next-ID-reservation: current userが「q581の後、WS114の装飾モード実装・GTK4確認を追加承認」とBから共有し、次Queue IDの予約を依頼。**q587をB1専用に予約**する。q581の後続で、Bがexact Phase/scope/criteria/承認原文/snapshot/依存を作成してAへcheckpointを送る。予約だけではactive attemptを投影せず、現在のB1 Queueはq581のまま。次の未予約main Queue IDはq588。browser2-evidence内の旧branch q587等はbranch取込の出典記録であり、本main Queueの履歴やclearanceと混同しない。
