# p172 checkpoint39 / q595-i01

Full C/component reviews of host-range-insert.c473, host-range-mutation.c401
and host-range-surround-contents.c367 original lines. Reviewed147/209,
remaining62 C/header; seven selected q595 reviews done, three remain.

Insert review covers direct splitText ToUint32 and Range insertNode, current
text changed during conversion, synchronous insertion hooks with actual GC,
no outer VM receiver/argument/C-stack retention, normal/EIO/genuine VM_THROWN
outcomes, committed prefix/suffix and Range repair after host failure, incoming
Fragment/foreign creator adoption and independent final reclamation. Installation
and native outcomes are individually checked; callback observer clears on every
exit. Generated inputs/result/native state are verified before pointer reads and
post-call roots, which start only after native callee root cleanup.

Mutation review covers optional FIFO weak insertion/data subscribers, complete
first-child publication, aliased full UTF16 replacement/live Range reset,
SIZE_MAX rejection preserving data/generation/nonzero endpoints/no notification,
surrogate append, removal-only subscriber exclusion and released graph GC.
Generated Text/Comment participants are checked and missing callback parent cannot
be dereferenced after a failed observation; acquired weak subscriptions share one
cleanup point and remain safe after Document/Range finalization.

Surround review covers1800 fully contained original children plus exact first/last
Text copies, native allocation GC with caller roots cleared, existing new-parent
child removal, inserted parent/selecting endpoints,1801 source records and1803
parent records retaining exact moved/cleared identities, last-root collection.
Construction wrapper/parent/state checks precede observations; only surviving
actual state is rooted for later inspection. Nested native node lookups have
named containers, script global publication verifies an existing wrapper.

All three main/script helpers check execution before teardown/source release.
Synthetic one-shot unwind becomes one shared forward cleanup point in mutation
and surround. Native operation returns are checked before post-call accounting.
Original normal corpora/strings/default production calls/root exclusions remain.

## Bounded verification

Scoped GCC14.2.0 -Wall/-Wextra/-Werror final fixture compile/link, exit0/warnings0,
current own actual engine objects; exact before/after argv and logs retained.
Native insert92/92/mutation36/36/surround16/16, timeout30s each, exit0/empty stderr:
tests.json. clang-format19.1.7 and definition/compound/pointer-loop/call/table
restorations: format.json. All original C strings equal, final source/object hashes
recorded, object texts differ. All209 source hashes reconcile, whitespace check pass.

Supporting checker exit1 only43 forward cleanup jumps (16 mutation/27 surround),
permitted by full C section14, insert zero findings, no other final findings:
style detail/summary and manual-goto-review.json. No allocation fault injection
or new target/boot/final API/ASan gate claimed; fixture-only engine rebuild unnecessary.
q595 item in-progress, whole p172 acceptance pending, WS incomplete, downstream
unselected. All fixtures exited; WIP/browser3 push, Issues/Project deferred.
