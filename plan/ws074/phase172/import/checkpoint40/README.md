# p172 checkpoint40 / q595-i01

Full C/component review of collection binding2029 original lines, collection
fixture404 and form-collection fixture337. Final sources2270/760/354 lines;
reviewed150/209, remaining59 C/header. All ten selected q595 files now verified.

## Verified native ownership repair

[Internal design/fix amendment](../../browser3/q595/amendment01.md) describes
two reproduced production GC failures and their repairs in the selected binding.
Original source1f3b3c90d fails each independent probe, exit2 with named failure;
[construction baseline](baseline-create.json)/[exact fixture](baseline-create-fixture.txt),
[conversion baseline](baseline-conversion.json)/[exact fixture](baseline-conversion-fixture.txt).
The callback refuses EIO before dereferencing missing private state, so the failure
is actual state reclamation rather than an assumed crash or sanitizer diagnosis.

Native construction registers DOM root, captured duplicate name and unpublished
state before their subsequent VM allocations; complete state precedes wrapper
publication, only acquired roots removed at the sole shared cleanup. Numeric
item, HTML named lookup, controls named lookup and radio string assignment retain
callee-owned state through actual user conversion and result publication.
Native receiver branding remains before conversion; wrapper/prototype/cache
identity and relevant-owner selection remain. Error and complete paths release
registered slots. No test-only production behavior, public interface or foreign
Phase scope changed. The original production strings and all23 constant
initializer blocks remain byte-identical.

The new fixture crosses the actual default8MiB allocation threshold using an
ordinary inert cell; state32bytes on this host crosses the threshold before
wrapper allocation. Collection count proves a real collection. Four real
valueOf/toString invocations run actual GC with no C-stack/caller receiver frame
or extra retaining root. Each returns its actual member/duplicate/assignment,
converts once, and loses its private state after callers drop observations.
Constructor state is likewise reclaimed after real embedding retirement.
Original16 collection and13 form assertions/corpora/root exclusions remain.
23 new string literal occurrences add only these regression inputs/labels:
[manifest](regression-literals.json), ordered original literals preserved.

## Full source review and bounded verification

[Manual coverage](full-standards-review.json) covers actual full authoritative
C/component texts, data/prototype/brand boundaries, uint32 keys, legacy named
visibility, ordinary expando/native refusal, live form/radio/option/table/image
membership, owner/prototype tracing, allocator errors, callback GC and teardown.
Ordinary forwards follow constant tables; only23 tables' required initializer
callbacks keep early one-line declarations under the existing user exception.
Definitions/arguments use tabs, semantic paragraphs separate checked construction
and cache publication, complex loop/condition layout restored. Main/script helpers
check execution before release, actual native nodes checked before dereference.

Final private plain host builder exit0/warnings0 after stable source, selected
GCC14.2 -Wall/-Wextra/-Werror compile/link exit0/empty logs. Actual native
collection37/form13/select12/table25/images10/properties65/form-owner56 =218
checks pass, timeout30s each/exit0/empty stderr: [selected](tests.json),
[affected](affected-tests.json), [build](build-result.json)/plain-build.log,
[exact compile/link argv](after-commands.json), [versions](versions.txt).
clang-format19.1.7/restored layout: format.json. Original/final actual source and
object hashes: object-code.json; .text changes expected. All209 source hashes
reconcile. No target/boot/final whole API/ASan campaign or fault injection claimed.

Supporting checker exit1 only33 forward shared-cleanup jumps,15 production and18
regression fixture, allowed by full C section14. Form fixture has zero findings;
[manual classification](manual-goto-review.json)/style-detail.txt/style-summary.txt.
Intermediate GCC warning-as-error rejected newly misplaced radio acquisition
before receiver validation; fixed before execution and rebuilt stable final source:
intermediate-build.json/intermediate-uninitialized-build.log. First repaired45
checks precede final root-release additions and do not replace final218 evidence.

q595 partial item ready for terminal reconciliation; whole p172 remains uncleared,
WS incomplete, downstream unselected. No subagents; WIP/normal browser3 push,
Issue/Project publication deferred. Prior build cleanup request already complete.
