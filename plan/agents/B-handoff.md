# Agent B → A pending projection checkpoint / 2026-10-02

B checkout: /home/awe/zedBSD-claude2 / codex/agent-b.
Integrate through commit60201ab8 (plus this handoff record). Shared Boards,
Master, history, registry, Guardrail and remote sync remain A-owned.

| Lane | Terminal / current Queue | Phase / acceptance | Evidence / integration |
| --- | --- | --- | --- |
| B1 | q581 finished/cleared → q587 active | p001 investigation cleared; p007 CSD implementation planned/dispatch; WS114 incomplete | q581 submitted21ce88c0 / merged9046f6fa / ACK52e1524e; q587 scope322d127a |
| B2 | q582 finished/cleared → q588 active | p011 cleared; p007 source/host/build partial only; whole/WS acceptance retained | submittedf2d5ca8e / mergedcea10fd7 / ACK3bed3013; q588 scope60201ab8 |
| B3 | q583 finished/partial cleared → q589 active | p017 whole uncleared, BUG125 reproduced/tracking; low-overhead finite diagnosis | submitted6a00fba6 / merged3ed1834c / ACKdf66db5e; q589 scope3b2d2924 |

## Exact approvals / scope

- q587: user explicitly instructed adding/executing decoration mode and GTK4 verification; [snapshot](B1/q587-approved-phase.md), SHA2566de8672526c942a4211b12369846a116e1049f6a38e6461cfc7a068e789a339f. New ws114-p007; changed p002/p003/p005 and WS events retain other row decisions/whole acceptance.
- q588: same-agent continuous Queue instruction applied to WS094 source conformance partial; [snapshot](../ws094/phase007/q588-approved-scope.md), SHA256a1faf4833c5289290f6ae2c42170d8be80ff0ab39b3520a04f033ec8f157a6f5. Max3h, full inventory/review, scoped owned violations/host4/build/boundary. Whole p007 physical/guest/boot retained; outside source findings only.
- q589: same instruction applied to next BUG125 discriminator; [snapshot](../ws099/phase017/q589-approved-scope.md), SHA25616005006ee6e1868d2ad67dc1c1328f27d27d1e43f02dd012fa31ad7877464c7. Max45min, 5 partial runs≤240s, remove pre-capture SSH snapshots only, first failure retained. Shared p076/product/parallel load excluded.

A reservations q587/B1, q588/B2, q589/B3 were read back from A coordination records. A reserved q590–q592 for its own successors; B does not allocate them.

## Durable outcomes / limitations

q581 all19 rows measured or concrete skip; actual Cairo/Vulkan software renderers,
move/resize and cross-client Unicode clipboard. Baseline double decorations and
restore growth still observed; these are evidence for new p007, not repaired by q581.
q582 host/build/style and prune/L1/drag/menu + final installed-image boot PASS;
old fixture kernel/current installed userland, fresh full image unbuilt, private
sysroot Vulkan header overlay1 difference. Guest stopped, final login PNG viewed.
q583 original5/handshake5 partialPASS, 2 prior popup symptoms not reproduced.
195asset hashes verified, all40 original PNGs retained. Overhead/serialized guest
limits preserved, no fix/resolution/whole p076 or C9 acceptance claimed.

## Resources / continuity / requested projections

B2/B3 owned guests stopped/readback, B1 q587 guest grant next. B3 q589 guest waits
for B1 stop; B2 q588 source/host/build proceeds independently. Same B1/B2/B3 agents
remain allocated and receive subsequent Queues without restarting/context reload;
only changes and missing next-scope material are reread. [B continuity](session-b.md).

Pending A projections: terminal cycle histories/Past Log, shared Queue rotation,
Master/registry/Outlook, BUG125 evidence link (retain tracking/reproduced), WS/Phase
remote mappings/comments (ws114 structural event on every changed Phase + WS).
GitHub publication/push remain deferred. No B edits to shared Boards/cache.
