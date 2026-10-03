# p172 checkpoint31 / q590-i01

Complete full C/component review of bind/select.c539 and bind/table.c575
original lines. [Inventory](review-inventory.json):130/209 reviewed;
[remaining79 C/header files](remaining.json), zero other files.

Select review covers exact native HTML local names/namespace vs mutable wrappers,
required option/optgroup union, HTMLElement-reference vs numeric-before coercion,
once-only current membership lookup, ancestor/not-found checks, same-node no-op,
actual group parent/reference insertion, five roots across conversions/host
notifications, defaultSelected vs dirty selectedness, peer normalization,
selectedIndex signed conversion and deliberately empty selection without fallback.
Seven initializer callbacks precede tables; four ordinary forwards follow all
registries. Actual strings/constant initializers/order are preserved.

Table review covers caption/head/foot actual interface validation, nullable
conversion and wrong-section refusal before old-child removal, first eligible
direct child vs nested tables, canonical caption/header/footer positions, repeated
create no-op, actual owner notifications, borrowed-method prototypes, checked
creation and retained old/new/anchor graphs across host GC. Six ordinary forwards
now follow the caption interface table. Allocation failure checks precede child
publication; both mutation callers check their results before releasing roots.
Exception helpers have explicit checked failure/success exits and unroot loops
have braces. Normal mutation, partial-error and callback ordering remain.

## Finite verification

- Own GCC14.2.0 plain rebuild: exit0/warnings0. [Log](plain-build.log).
- Actual native select78/78, option-state20/20, table-mutation42/42; page
  select-options72/option-selectedness73/table-structure68 PASS/zero FAIL,
  exit0/empty stderr. [Final commands/results](tests.json).
  Initial successful checks ran while the builder still linked other fixtures;
  final checks were repeated after confirmed builder exit to guarantee freshness.
- All original C literals and constant registries unchanged; actual object text
  differs after checked branches. [Hashes](object-code.json),
  [initializer/order receipt](constant-initializers.json).
- Scoped clang-format19.1.7/mandatory definition and compound restoration
  [receipt](format.json); [style checker](style-summary.txt) zero findings;
  full manual review and git diff --check passed.

All209 current hashes reconcile. q590/p172 in-progress, WS incomplete;
79 full reviews/final conformance required. No new target/guest/ASan/downstream
campaign. GitHub publication deferred; all owned checks ended.
