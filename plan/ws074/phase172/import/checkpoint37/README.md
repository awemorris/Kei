# p172 checkpoint37 / q595-i01

Full C/component reviews: host-range-boundary.c463 and
host-range-data.c562 original lines. Reviewed142/209, remaining67 C/header.
These are two of q595's exact ten selected files; eight selected reviews remain.

Boundary review covers actual native setStart and compareBoundaryPoints,
valueOf conversion with collector pressure and no outer VM receiver/argument
frame, old/new endpoints and otherwise unbound XML creator, three outcomes
(normal/EIO/genuine VM_THROWN), current length validation, post-call sole state
root and final reclamation. Generated wrapper/private state/node observations
are guarded. Expected error/throw convention is checked immediately; fixture
source storage and acquired root release on failure. Callback observer clears
before the C invocation ends. No additional retaining root inside conversion.

Data review covers genuine native UTF16 interval replacement/deletion/insertion,
clamped SIZE_MAX arithmetic and byte overflow, aliased input, exact unpaired
surrogate units, unchanged allocation on deletion, independent Range and detached
clone offset repairs, weak ordered optional data/removal subscribers, current
owner migration on adoption, genuine Comment and eventual released graph GC.
Script return checks precede source release, generated CharacterData is verified,
and36 synthetic-loop breaks become forward jumps to one shared cleanup point.
Acquired weak subscriptions unwind safely even after original Document finalization.
No change to actual mutation calls, normal scripts or expected observations.

## Verification and limits

- GCC14.2.0 scoped actual fixture compile/link against current own engine objects,
  -Wall/-Wextra/-Werror, exit0/warnings0. before/after-commands.json retain exact
  argv; before-compile.log/compile-link.log have no diagnostics.
- Final native boundary89/89 and data100/100, timeout30s each, exit0/empty stderr:
  tests.json and named stdout. No production rebuild needed for fixture-only edits.
- clang-format19.1.7, definition tabs, compound-clause/split-call layout and constant
  initializer restoration: format.json. All original C strings equal, final source
  and actual object-text hashes recorded; object texts differ: object-code.json.
- Supporting checker exit1 solely36 forward jumps to partial_case's one cleanup
  label, permitted by full C section14; style-detail.txt/style-summary.txt and
  manual-goto-review.json. Boundary has zero findings; no exception or waiver.
- Full control/lifetime review and git diff --check pass. Real GC/root exclusions
  and default bindings remain. No allocation fault injection or new target/boot/
  final boundary/public-client/ASan conformance campaign claimed.

All209 source hashes reconcile. q595 partial item in-progress, whole p172 remains
uncleared until all remaining reviews/final gates; WS incomplete, downstream
unselected. WIP/normal origin/browser3 push; Issue/Project publication deferred.
q594 cleanup remains completed; unrelated prior scratch logs are not q595 evidence.
