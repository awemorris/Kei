# p172 checkpoint27 / q590-i01

Complete full C/component review of host-traversal.c362 and host-removal.c654
original lines. [Inventory](review-inventory.json):124/209 reviewed;
[remaining85 C/header files](remaining.json), zero other files.

Traversal review covers production script conversions, candidate/filter callback
GC with conservative roots excluded, original iterator argument vs repaired
cursor, weak subscriber reclamation, foreign child retention solely by current
Node and reclamation after its release. Observed VM completion/Node owners are
now checked before native dereference; unexpected results return EIO after full
embedding teardown. Failed script/source paths release converted storage after
immediate status guards. No script, expected assertion or collector mode changed.

Removal review covers old links/FIFO delivery, detached/unrelated silence,
released tokens, rejected cross-heap/Document adoption, four actual back-and-forth
root migrations, Document-first/subscriber-first collection and both normal/large
heap teardown orders. Tokens never trace weak observer/Document; every C stack
counter outlives its observer finalizer. One table callback declaration remains
before its initializer; ordinary forwards follow. Adoption failures now check
and release current token ownership immediately before either heap teardown.

## Finite verification

- Scoped GCC14.2.0 fixture compiles/link against actual own production objects:
  exit0/warnings0. [Commands](compile-link.json), [log](compile-link.log).
- Actual traversal9/9 and removal37/37, exit0/empty stderr.
  [Commands/results](tests.json). No weakened expectations or new alternate mode.
- All original script/assertion C strings unchanged; final object text differs
  after guarded failure paths. [Hashes](object-code.json).
- Scoped clang-format19.1.7, mandatory definition tabs/compound layout
  [receipt](format.json); [style checker](style-summary.txt) zero findings;
  full manual review and git diff --check passed.

All209 hashes reconcile. q590/p172 remain in-progress and WS incomplete;
85 full reviews/final conformance remain. No additional target/guest/ASan or
browser downstream campaign. GitHub publication deferred; all owned checks ended.
