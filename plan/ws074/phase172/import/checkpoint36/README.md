# p172 checkpoint36 / q594-i01

Full C/component reviews of css/rule-model.c662 and
host-range-delete-contents.c299 original lines. Full review140/209;
remaining69 C/header, zero other entries. Combined receipt covers the two
remaining selected files in q594's exact five-file partial scope.

CSS model review covers immutable copied UTF16 source ownership, live order
and retained stable IDs, retired sources until model destruction, tolerant
initial parse versus strict single-rule insertion, independent candidate parse
and failure rollback, grouping and unsupported at-rules, tokenized strings/
escapes/comments, nesting128 and retained entries65536 bounds, UINT32_MAX IDs,
EOF closure joining, absent lookup output reset and borrowed source lifetime.
Allocation/check and initialization/publication paragraphs are separate;
lookup and closed-block scan finish with explicit final success. No public API,
source text, parser behavior or stable identity/lifetime contract changes.

Range fixture review covers direct registered native deletion of1800 actual
children, real7MiB GC pressure, construction root cleared before invocation,
no caller receiver frame or conservative C-stack retention, private state
collapse, all FIFO MutationObserver records retaining exact detached identities,
and collection after record/source release. Status and generated wrapper/cell/
node checks precede observations; the synthetic do/break unwind becomes one
shared forward cleanup point. Actual retained state is checked before allocating
its inspection wrapper. No extra root during the operation or alternate callback.

## Finite verification

- Final complete private plain host build, GCC14.2.0 with -Wall/-Wextra/-Werror,
  exit0/warnings0. The initial builder overlapped final Range fixture edits;
  final-build.log is a completed rerun after stable source, before native tests.
- Actual CSS model44/44, CSSOM mutation12/12, Range deletion11/11: tests.json,
  all exit0/empty stderr, each timeout30s. Earlier scoped Range compile/link and
  native11 also passed, retained as range-pre-final-* receipts. Final native
  result uses the final builder's fixture, not stale cached test output.
- clang-format19.1.7 with definition tabs/compound/nonstandard loop and inert
  constant-table restoration: format.json. Final source and actual object-text
  hashes in object-code.json; both object texts differ, all original C strings
  identical. Final diff read and git diff --check pass.
- Supporting checker exit1 only18 forward jumps to delete_case's sole cleanup
  label, allowed by full C section14: style-detail.txt/style-summary.txt and
  manual-goto-review.json. No other findings; no exception or failure waiver.

All209 final source hashes reconcile. Both files satisfy their full scoped
review criteria; all five selected q594 files now reviewed. This checkpoint
records execution before Queue terminal reconciliation. Whole p172 remains
uncleared pending69 full reviews and final gates, WS incomplete, downstream
unselected. No target/boot/ASan campaign or allocation-failure injection claimed.
Issue/Project publication deferred. Requested post-commit cleanup was completed
and pushed at e209f9bc1, with committed q594 cleanup audit/result.
