# p172 checkpoint26 / q590-i01

Complete full C/component review of bind/iterator.c708 original lines.
[Inventory](review-inventory.json):122/209 reviewed;
[remaining87 C/header files](remaining.json), zero other files.

Review covers receiver Document/prototype snapshots, actual Node roots and mask
coercion order, fully initialized heap state, conservative caller construction,
traced permanent/candidate/pending/filter edges and weak removal subscription
ownership. Subtree removal repairs both cursor directions; filter rejection,
callback deletion/regrafting, coercion reentry, throws and exhaustion preserve
permanent repair without publishing an unsuccessful candidate. Removed child
XML Documents retain their factory prototype snapshot; detach remains harmless.

Only ten callbacks named by constant initializers retain pre-table declarations;
seven ordinary forwards now follow all four tables. Argument coercion/allocation,
exception helpers, XML snapshot and wrapping receive immediate status guards.
The active flag and pending/candidate cursors reset on both callback outcomes;
normal accepted-reference publication order remains unchanged.

## Finite verification

- Own GCC14.2.0 plain rebuild: exit0/warnings0. [Log](plain-build.log).
- Actual native traversal9/9 and weak-removal37/37; NodeIterator page60
  PASS/zero FAIL/exit0/empty stderr. Includes callback deletion, throwing-filter
  repair/resume, finite regrafting and retired-child factory behavior.
  [Commands/results](tests.json).
- C strings and all constant initializers/order unchanged; actual object text
  differs after guarded control flow. [Hashes](object-code.json).
- Scoped clang-format19.1.7/definition tabs [receipt](format.json);
  [style checker](style-summary.txt) zero findings and git diff --check passed.
  Full manual review supplies lifetime/control-flow checks beyond automation.

All209 final hashes reconcile. q590/p172 remain in-progress, WS incomplete;
whole final conformance and87 full reviews remain. No new target/guest/ASan or
browser downstream campaign. GitHub publication deferred; owned checks ended.
