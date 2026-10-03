# Browser3 / q590-i01 checkpoint25

Complete full C/component review of net/data.c675 original lines.
[Inventory](review-inventory.json):121/209 reviewed;
[remaining88 C/header files](remaining.json), zero other files. q590/p172 remain
in-progress; downstream gates stay closed.

Review covers fragment-free data URL ownership, comma/ASCII declaration trimming,
percent/forgiving-base64 input and padding, exact decoded bytes, MIME token and
quoted-string validation, duplicate case-folded names, quoting/escaping and
MIME-only fallback separated from body/allocation errors. Each fallible write
now has an immediate guard; all temporary C buffers release on either outcome,
and failed outputs retain idempotent empty release semantics.

## Reproduced MIME corrections

[Internal amendment03](../../browser3/q590-amendment-03.md) records two defects:
quoted values containing ;b=x wrongly suppressed a subsequent actual b=y, and
leading subtype whitespace was wrongly accepted. The primary
[MIME parsing/serialization algorithm](https://mimesniff.spec.whatwg.org/#parsing-a-mime-type)
was checked2026-10-02. Values inside quotes are not parameter names; only trailing
subtype whitespace is removable. No broader MIME policy/product feature change.

Four ordinary native input/output cases cover quoted delimiters, folded actual
duplicates, escaped quote and invalid subtype whitespace. Before fixes, actual
baseline39091b2b9 engine objects produce76/80 and exit1 with four MIME failures:
[commands](regression-before.json), [corpus patch](regression-before.patch) and
[raw stderr](data-before.stderr). The earlier three-case exploratory run was
74/77/exit1; the durable four-case snapshot fully reproduces both corrections.
Final native80/80 and LeakSanitizer80/80 exit0/no leak establish the correction.

The existing host-data-url supporting fixture was already fully reviewed in
checkpoint11. Its complete file/changed corpus have been revalidated against
C/component rules and its inventory hash/receipt updated without counting it
again: [receipt](supporting-fixture.json). Original mechanism/expectations stay
unchanged except four added real cases.

## Finite verification

- Own GCC14.2.0 plain host rebuild: exit0/warnings0. [Log](plain-build.log).
- Final native data80/80 and actual local/HTTP resource105/105. Focused
  [LeakSanitizer link](lsan-link.json) against actual plain engine objects:
  80/80/exit0/empty stderr. This checks allocator leaks, not ASan memory bounds.
  [Commands/results](tests.json). Initial resource invocation lacked required
  directory/server arguments and exited2 before assertions; corrected driver
  passed105/105. The failed invocation is retained explicitly.
- Actual C string literals unchanged, with character literals/comments handled
  separately by the scanner. Object text differs after MIME corrections and
  guarded cleanup. [Hashes](object-code.json). No executable equality claim.
- Scoped clang-format19.1.7 plus mandatory definition tabs/compound layout:
  [receipt](format.json). [Style checker](style-summary.txt) exits1 with14 goto
  findings only; each is a forward jump to its function's one cleanup label
  allowed by full C section14: [manual classification](manual-goto-review.json).
  Full manual review and successful git diff --check accompany automation.

## Publication correction

WIP050031f14 published source/raw evidence before inventory saving finished:
the hash guard detected that the supporting fixture was previously reviewed.
This follow-up supplies the missing inventory/remaining/summary/projections and
corrects its classification to checkpoint11 revalidation. Source and successful
build/test results are unchanged; no acceptance was granted by the partial push.
All209 final hashes reconcile in this completed receipt.

No new target/guest/ASan or downstream campaign. Remaining88 full reviews/final
conformance required; GitHub publication deferred. All owned processes ended.
