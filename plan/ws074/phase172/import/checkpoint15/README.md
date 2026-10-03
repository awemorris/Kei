# Browser3 / q590-i01 checkpoint15

Three complete files were reviewed against the full C and browser component
standards: bind/style-context.c, bind/implementation.c and the real native
host-frame-viewport.c fixture. [Inventory](review-inventory.json) retains all209
current source hashes and prior review receipts:100 reviewed,109 remaining
C/header entries, with all60 other entries already reviewed. Whole p172 is
still in-progress and downstream browser gates remain closed.

## Finding and correction

The parent geometry callback can remove an iframe and retire its child Window.
bind_frame_viewport already handles this, but bind_style_context_engine did not
recheck retirement before allocating and lending a new CSS engine. The existing
native fixture now enters through this actual caller, removes the iframe and
collects with conservative stack scanning disabled. Before the fix both new
assertions fail, exit1/10 of12 checks: [before](viewport-before.log). The narrow
post-callback detached guard prevents allocation/publication and preserves the
already initialized NULL output: [internal design amendment](../../browser3/q590-amendment-01.md).
This is an actual lifetime correction, distinct from conformance-only changes.

Review covers CSS engine-before-sheet destruction, owning parser/media arenas,
temporary units cleanup, ancestor storage and managed Window retirement;
DOMImplementation's traced Document/prototype snapshot, native branding and
ordered name/public/system conversions; and fixture installation/teardown,
real callback GC roots and collection after retirement. Necessary callback
declarations precede constant initializers; other declarations follow tables.
Fallible parsing/script calls are checked before temporary buffer release.
Fixture setup rejects absent/non-element bindings, realms and child Windows.
Public exports, callback timing, active-child behavior and component boundaries
are preserved. No foreign Phase or acceptance criterion changes.

## Verification

- Own incremental host build: exit0, warnings0 using GCC14.2.0, original plain
  flags and `BROWSER_HOST_BUILD=build/browser3-p172`. [Build log](plain-build.log).
- Final rebuilt native viewport12/12, iframe lifetime148/148, XML Document16/16
  and CSSOM mutation12/12: all188 checks pass, exit0, empty stderr.
  [Commands/results](tests.json). The related three tests account for176 checks.
- DOMImplementation before/final production object text is exactly equal.
  style-context differs because of the documented lifetime guard; no blanket
  object-equality claim is made. [Object hashes](object-code.json).
- Scoped clang-format19.1.7 then mandatory definition-argument tabs and semantic
  paragraph separation: [receipt](format.json). Full manual review accompanies
  supporting [style checker](style-summary.txt), exit0, and `git diff --check`.

[Remaining109](remaining.json) is the next exact continuation. No new guest,
target rebuild, ASan campaign or downstream suite ran. Earlier evidence remains
retained and whole final validation is still required. GitHub Issues/Project
publication remains deferred; Git push is separate. q590 remains active within
its original finite timebox; this checkpoint does not clear the attempt.
