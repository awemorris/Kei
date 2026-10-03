# p172 checkpoint30 / q590-i01

Complete full C/component review of host-css-rule-model.c423 and
host-cssom-mutation.c337 original lines. [Inventory](review-inventory.json):
128/209 reviewed; [remaining81 C/header files](remaining.json), zero other files.

Rule model review covers immutable copied source, escaped/quoted boundaries,
recognized/discarded groups, actual flattened cascade, stable/retired addresses,
insert/delete ordering, invalid index/selector/multiple-rule/trailing input,
unsupported at-rules, EOF block/import closure, supplementary UTF16 and bounded
curly nesting. Every expected success/refusal is now classified before borrowed
metadata/count observations. Required borrowed source storage/length is checked
before indexing; every joined text/sheet/engine/model failure releases its owner.
Original normal assertion corpus/count and native behavior remain intact.

Mutation review covers ordinary Page/font lifetime, managed child style source,
real index-coercion retirement, no-stack-root production threshold collection,
converted rule survival, obsolete model independence, current model rebuilding
and eventual old model/Document/realm reclamation. Unexpected Node/Text/source
observations now fail before native dereference. The synthetic do-once unwind
becomes one standard forward cleanup point; only successfully registered roots
are removed, callback context clears and conservative embedding base restores.
Actual collector pressure, root exclusion and all original expected observations
remain unchanged. No engine test switch or production change.

## Finite verification

- Scoped GCC14.2.0 fixture compiles/link against actual own current production
  objects: exit0/warnings0. [Commands](compile-link.json), [log](compile-link.log).
- Actual model44/44 and mutation12/12, exit0/empty stderr, including real
  threshold/callback collection and source/child retirement.
  [Commands/results](tests.json).
- All original script/assertion C strings unchanged; actual fixture object text
  differs after failure guards. [Hashes](object-code.json).
- Scoped clang-format19.1.7/mandatory definition and compound restoration
  [receipt](format.json), full manual review and git diff --check.
  [Style checker](style-summary.txt) exits1 only for14 forward cleanup jumps
  in mutation_case, each to its one shared cleanup label permitted by full C
  section14: [classification](manual-goto-review.json).

All209 current hashes reconcile. q590/p172 remain in-progress, WS incomplete;
81 full reviews/final conformance required. No target/guest/ASan/downstream
campaign. GitHub publication deferred; all owned checks ended.
