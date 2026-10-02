# q587 scope amendment01 / GTK4 move/resize release

2026-10-02 / B main technical reconciliation, current user's explicit CSD
implementation + GTK4 operation verification instruction remains the authority.
Original approved snapshot is preserved at plan/agents/B1/q587-approved-phase.md
(SHA256 6de8672526c942a4211b12369846a116e1049f6a38e6461cfc7a068e789a339f).
Same q587-i01, same3-hour deadline2026-10-02 10:41:58 UTC, same acceptance criteria.
No new capability or independent bug campaign is added.

## Evidence / reason

Current q587 source/runtime already draws standard GTK4 with CSD only, GL text,
menu and modal. Fresh GTK4 PID791 move delivers button state1 and accepted xdg
move, but the QMP release has no client state0; subsequent axis arrives without
GTK scroll callback. [Receipt](../evidence/q587/move-release-check.log).
Thus the existing move/resize-followed-by-input criterion fails until the release
path is corrected. Hiding decorations alone cannot satisfy the user's requested
GTK4 operation. q581's similar limitation is retained, not retroactively fixed.

## Exact additional source / design constraints

Add seat.c and toplevel.c to q587's source ownership, with narrowly needed zwl.h
state and objects.c teardown (objects.c was already allowed). Capture the origin
surface/client and actual initiating button only for an accepted client xdg
move/resize request. When the compositor consumes the matching release ending
that operation, send the release once to the original live client's pointer,
including when pointer ends outside the window. Retire state at operation end
and on surface/client destruction. Reuse existing seat delivery/protocol error
and object lifecycle contracts; no broad input/grab redesign.

Server-owned SSD frame/titlebar operations must not fabricate a release to a
client that did not receive the press. Preserve popup/DnD/lock handling and native
SSD operations. Distinguish client-requested operations rather than infer from
CSD appearance or the current pointer hit target. Avoid duplicate release and
stale borrowed identity accesses. No HAL/public platform API changes.

## Added focused verification

Actual GTK CSD move then scroll/click/key, resize then scroll/click/key, and resize
ending outside the original surface must show one matching button release and
working callbacks. A canceled/destroyed operation and native SSD titlebar/frame
operation verify no stale-client/phantom/duplicate delivery. Existing q587 wire,
GTK renderer/native regression, warning0 builds/last boot and full changed-source
review remain required. Finite investigation/timebox unchanged; failure leaves
p007 uncleared with concrete resume condition.

Main informed B1 before implementation and records this delegated technical
refinement durably. A shared projection/event publication remains pending.
