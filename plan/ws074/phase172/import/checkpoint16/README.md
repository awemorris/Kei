# Browser3 / q590-i01 checkpoint16

Complete full C/component review of host-image-geometry.c231,
host-document-images.c234, host-cssom.c253 and host-select-collection.c259
original lines. [Inventory](review-inventory.json) has104/209 full reviews,
[remaining105 C/header entries](remaining.json), zero other pending entries.
q590 and whole p172 remain in-progress; downstream gates stay closed.

Review follows actual Page fonts/layout/child geometry callback retirement,
Document.images SameObject tracing and collection-only retired child retention,
CSSOM source/model replacement and callee-rooted threshold GC, and XML select
options caches/live mutation/final collection. Registered root addresses remain
valid until removal; the Document.images getter error now removes its local
root before returning. Select checks a post-GC cell before reading its cache.
Fixture setup checks returned bindings/types, child realms/owners, parsed tree
nodes and sheet wrappers before dereference. Return codes are checked directly
after script/model calls; converted units and Page/Window/realm/heap unwind
preserve their original order. Void observers explicitly finish successfully.
Purpose paragraphs and variable naming distinguish the actual select graph.

All script inputs, check names and other string literals are exactly unchanged:
[source/object evidence](object-code.json). All four fixture object texts differ
because of explicit failure safeguards and control-flow cleanup; no executable
equality is claimed. No production source changed in this checkpoint.

## Finite verification

- Each final fixture compiled and linked warning0 with retained own GCC14.2.0
  plain flags and the actual engine objects rebuilt at checkpoint15.
- Rebuilt image geometry10/10, Document.images10/10, CSSOM12/12 and select
  collection12/12 pass:44 checks, all exit0/empty stderr. [Commands/results](tests.json).
  Exact link commands concatenate each prefix, shared [object list](link-objects.json)
  and suffix; no alternate engine implementation or test-only production mode.
- Scoped clang-format19.1.7 then mandatory definition argument tabs:
  [receipt](format.json). Manual full review accompanies supporting
  [style checker](style-summary.txt),0, and successful `git diff --check`.

No additional production rebuild, target boot, ASan or downstream campaign was
needed for these fixture-only changes. Whole final conformance remains required.
GitHub publication remains deferred; normal browser3 push is separately allowed.
