# p172 checkpoint34 / q590-i01

Full C/component review of host-click.c280 original lines. Inventory135/209;
remaining74 C/header, zero other entries.

Reviewed direct native activation with no script receiver frame, removed previous
radio and parent event-path retention, listener-triggered actual GC with C
stacks excluded, cancellation after group membership changes, dirty checkedness,
released click guard and eventual previous/parent/target reclamation. No new
fixture roots or production engine switch.

Fixture script/whole-case errors are checked before releasing input/heap;
native Event/target/parent/previous selection and generated form element are
checked before dereference. Post-collection Event and activation target are
checked before cancellation/state inspection. Nested argument lookup has a
named value and success exits are explicit. All original scripts/assertion
strings and ten normal checks remain unchanged.

## Finite verification

- Scoped GCC14.2.0 compile/link against actual own current production objects:
  compile-link.log, exit0/warnings0.
- Actual click10/10, exit0/empty stderr: tests.json and click.stdout/stderr.
- clang-format19.1.7 plus mandatory definition/compound restoration: format.json.
  Supporting style-check0, full final manual diff and git diff --check pass.
- Source/object hashes and all unchanged C strings: object-code.json.

All209 hashes reconcile. q590/p172 remain in-progress, WS incomplete;74 full
reviews and whole final gates required. Target/boot/ASan/downstream campaign
not rerun. All owned checks ended; Issues/Project publication deferred.
