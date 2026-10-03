# Browser3 / q590-i01 checkpoint21

Complete full C/component review of dom/serialize.c554 and js/builtin_symbol.c571
original lines. [Inventory](review-inventory.json):115/209 reviewed;
[remaining94 C/header files](remaining.json), zero other files. q590 and p172
remain in-progress; downstream gates stay closed.

Serialization review covers template contents, ordinary/void/foreign elements,
attribute namespace prefixes, exact UTF-16 entity substitution, raw text and
scripting-dependent noscript, actual PI target/data, comment/DOCTYPE and partial
allocation failure. Each write now has an immediate failure exit and an explicit
success result; escaped temporary units release on both outcomes. Original
serialized bytes, failure codes and partial-output behavior are preserved.

Symbol review covers all13 well-known entries, constructor/prototype/registry
root publication, primitive and wrapper brand checks, optional descriptions,
registered identity and reverse lookup, tag/species defaults and absent-global
handling. Registration order/table are exact: [comparison](symbol-order.json).
Fallible chained operations become individual guards; VM exception propagation
and normal values are preserved. No API, callback/lifetime contract, new test
control or registry policy changes.

## Finite verification

- Own GCC14.2.0 plain host rebuild: exit0/warnings0. [Log](plain-build.log).
- Actual iteration47 lines and markup22 lines exactly equal checked-in expected
  output. Native XML nodes32, attribute namespaces25 and VM factory GC20:
  77/77 pass, all exit0/empty stderr. [Commands/results](tests.json).
- C string literals equal in both files. Symbol actual object text is identical;
  serializer text differs after explicit guards and cleanup rearrangement.
  [Exact hashes](object-code.json). No blanket source/object equivalence claimed.
- Scoped clang-format19.1.7 then mandated definition tabs/compound layout:
  [receipt](format.json). Supporting [style checker](style-summary.txt)0 and
  successful git diff --check accompany full manual review.

No new target/guest, ASan or downstream campaign ran. Remaining94 full reviews
and final whole conformance stay required. GitHub publication deferred;
all owned build/test processes completed.
