# p172 checkpoint32 / q590-i01

Full C/component review of host-option-state.c291, host-table-collection.c268
and host-select.c423 original lines. Inventory133/209; remaining76 C/header.

Reviewed numeric setter conversion/error ownership, six select addition cases
including grouped anchors, borrowed retired-child owners and actual insertion
host failure, four table-family cache identities and sole cells-wrapper
reachability. Observer state uses integer addresses, native invocations have
no independent script receiver frame, collection deliberately excludes C stacks,
and last root release must reclaim original owner/member/argument/cache graphs.

Fixture errors are checked before releasing borrowed input or publishing native
observations. Generated Node/parent/cache observations are checked before
access; a collected table is checked before cache dereference. A single shared
teardown restores the embedding stack contract and expires observer callbacks.
No additional retaining roots or production changes. Existing script/assertion
strings and all normal counts remain unchanged.

## Finite verification

- Scoped GCC14.2.0 -Wall/-Wextra/-Werror fixture compile/link against current
  actual own production objects, exit0/warnings0: compile-link.log.
- Actual option-state20/20, table-collection25/25 and select78/78, all exit0,
  empty stderr: tests.json and individual stdout/stderr.
- An initial compiler error in the newly added fixture guard used Node.kind;
  corrected to actual Node.type before successful compile/tests. Initial log
  retained as compile-link-first.log; no failed source was committed.
- clang-format19.1.7 plus full-standard definition tabs/compound restoration:
  format.json. git diff --check passes and complete final diff manually read.
- Style checker exits1 solely for19 forward jumps, each to that function's
  single cleanup label permitted by full C section14: style-summary.txt and
  manual-goto-review.json. No other findings.
- Source and actual object-text hashes/C-string equivalence: object-code.json.

All209 source hashes reconcile; whole p172/q590 remain in-progress, WS incomplete.
Remaining76 full reviews and whole final gates remain. No target/guest/ASan or
new downstream campaign. All fixture processes ended; Issue publication deferred.
