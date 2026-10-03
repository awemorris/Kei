# Browser3 / q590-i01 checkpoint19

Complete full C/component review of js/builtin.c441 and vm/class.c504 original
lines. [Inventory](review-inventory.json):111/209 reviewed;
[remaining98 C/header files](remaining.json), zero other files. q590 and whole
p172 remain in-progress; downstream gates remain closed.

Review follows all19 ordered builtin installers, native function/constructor
publication, configurable non-enumerable accessor pairs, boxed strings/callees,
root-array size arithmetic, retained input/unpublished Array slots and integer/
array-like length conversion. Each installer now has an immediate error guard
and meaningful purpose paragraph; [original installer order](installer-order.json)
is exactly preserved. Array generation uses one shared forward cleanup label
for every registered root and its C slot table, publishing only populated output.

Class review follows absent/null/constructor heritage, inherited prototype
getters, native constructor/home edges, data/accessor descriptors, super's actual
this receiver, own private brand lookup, writable fields, getter/setter errors,
duplicate initialization and instance brand copying. Public introduction lines
and explicit failure/success exits conform to full C rules. Normal messages and
attributes remain unchanged. Native name/message snprintf results now reject
a negative formatting result with EIO before proceeding; bounded truncation and
successful names preserve prior behavior. This necessary fallible-call safeguard
is disclosed rather than claiming all possible failure behavior is identical.

## Finite verification

- Own GCC14.2.0 plain host rebuild: exit0/warnings0. [Build log](plain-build.log).
- Actual VM factory20/20 and heap31/31 native checks pass, exit0/empty stderr.
  Existing class17-line and builtins30-line JS stdout match checked-in expected
  output exactly, exit0/empty stderr. [Commands/results](tests.json).
- Scoped clang-format19.1.7 then mandatory definition argument tabs:
  [receipt](format.json). [Style checker](style-summary.txt) reports only3 goto
  findings, exit1. Each is one forward jump to Array generation's shared cleanup
  label, permitted by full C §14: [manual disposition](goto-review.json).
  No rule waiver; `git diff --check` passes.
- Original string literals are equal; both object texts differ after explicit
  failure/control-flow changes: [hashes](object-code.json). No blanket object
  equality or injected negative-snprintf execution is claimed.

No new target/guest, ASan or downstream campaign ran. Remaining98 full reviews
and final whole conformance remain required. GitHub publication stays deferred;
own rebuild and bounded tests have exited.
