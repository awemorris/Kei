# p172 checkpoint29 / q590-i01

Complete full C/component review of js/builtin_json.c1386 original lines.
[Inventory](review-inventory.json):126/209 reviewed;
[remaining83 C/header files](remaining.json), zero other files.

Review covers JSON global/parse/stringify installation order, coerced UTF16 text,
JSON whitespace, strict punctuation/literals/number grammar, negative zero,
container recursion, Unicode escapes, reviver key snapshots/delete/define,
toJSON/replacer ordering, primitive wrappers, property-list coercion/deduplication,
array null placeholders vs omitted object members, indentation/rollback, circular
objects and paired/lone-surrogate output. Interned property keys are rooted by
the actual atom table (vm/heap.c marks atoms); native VM temporaries preserve
ordinary conservative caller/realm ownership. No new collector policy.

Every fallible step now has an immediate status guard; status-aware chained
calls/loop heads are replaced with explicit failures. One cleanup point per
owned writer/string/key snapshot restores caller indentation and releases C
storage on failures. Successful objects alone leave the cycle stack; failures
still abort the enclosing writer. Top-level failures still produce undefined.
The reviver key snapshot releases before its enclosing callback as before.

Necessary failure safeguards check vm_key_from_ascii's actual VM_VALUE_EMPTY
sentinel before reading toJSON, and negative snprintf results before formatting
is consumed. All original normal/error C strings, limits and callback ordering
remain. No actual allocation/formatter fault injection is claimed for these guards.

## Finite verification

- Own GCC14.2.0 plain rebuild: exit0/warnings0. [Log](plain-build.log).
- Existing collections46, regexp30, date31 expected lines and nine focused JSON
  exception/recovery lines:116 exact lines, exit0/empty stderr.
  [Commands/results](tests.json), [additional inputs](json-error-paths.js),
  [independent expectations](json-error-paths.expected).
- Actual reviver/replacer/toJSON/property-getter/replacer-list-getter/gap coercion
  throws preserve their exact exception and allow subsequent parse/stringify;
  eleven malformed strings, circular structure and lone surrogates also verify
  normal exceptions/output. No alternative engine/test-only mode.
- Focused [LeakSanitizer link](lsan-link.json) against actual plain objects:
  same nine lines/exit0/empty stderr/no leak. Allocator leak coverage only;
  production objects are not instrumented for ASan memory bounds.
- Original C literals unchanged; actual object text differs after checked control
  flow. [Hashes](object-code.json). No executable-equality claim.
- Scoped clang-format19.1.7 with mandatory definitions/compound/symmetric branch
  restoration [receipt](format.json), full manual review and git diff --check.
  [Style checker](style-summary.txt) exits1 only for48 forward cleanup jumps,
  each to its function's single cleanup label permitted by full C section14:
  [manual classification](manual-goto-review.json).

Existing2000-depth/400-numeral limits and conservative native-call requirements
remain; no Test262/full JSON conformance claim. All209 current hashes reconcile.
q590/p172 remain in-progress, WS incomplete;83 full reviews/final conformance
remain. No target/guest/ASan/downstream campaign. GitHub publication deferred;
all owned checks ended.
