# p172 checkpoint41 / q596-i01

Full C/component review of function/environment/call implementation and the
existing VM factory native fixture (335→942, 811→1250 lines). Review152/209, remaining57
C/header; all209 source hashes reconcile with checkpoint40. q596 whole item
remains in-progress, p172 not cleared, WS incomplete, p100 unselected.

## Reproduced ownership defects and scoped repairs

[Internal amendment01](../../browser3/q596/amendment01.md) records the unchanged
scope and internal design revision. Original bca821b4a producer loses the parent
(exit2), rejects native output (exit2), and crashes on bytecode/closure generation
(signal11). Exact four-mode default-GC probe: baseline-function-probe.txt and
baseline-functions.json retain source/fixture hashes, compile/run argv and actual
failure output. First factory repair passes all four under the same threshold:
repaired-functions.json/stdout/stderr.

The original dispatcher after only factory repair loses all five direct or
suspended participants during actual nested native collection, exit1. Separate
baseline-native-probe.txt/baseline-native.json retain the failure;
factory-only-repair.patch and baseline-native-producer.txt preserve its precise
intermediate producer. Repaired dispatcher retains all five, exit0:
repaired-native.json/stdout/stderr. Integer addresses are only observations,
never extra roots; missing participants are not dereferenced.

Factories retain managed realm/code/environment before allocation, then private
function/name/prototype cells until publication. Parent allocation arithmetic
is host-size bounded. Call/constructor/prototype lookup retain direct input
cells and managed caller realm through getters, reentrant execution, throw
transport and publication. Native entry additionally retains suspended
callee/new.target while fields are replaced, restores them before root release,
and unwinds only registrations actually acquired. Full allocation/failure paths
were manually reviewed; no production test-only switch, threshold or API changes.
Original constructor/class/generator/async/intrinsic/throw conventions retained.

Default host-vm-factory incorporates four real threshold regressions and nested
native GC, including initialized slots, exact name/length, actual22 bytecode/
capture result, restored realm state and later input/output collection. Caller
construction and conservative roots are dropped **before** each tested callee;
output receives its caller root only **after** verified complete publication.
Original Array/object inputs/assertions and ordered C literals remain; only
regression inputs/labels are added: regression-literals.json. Production C
strings and both original cell initializer blocks are byte-identical:
object-code.json. No Bug-transfer waiver.

## Final verification and authoritative review

Own stable final plain host build exit0/warnings0/errors0:
build-result.json/plain-build.log. Scoped GCC14.2 -Wall/-Wextra/-Werror
compile/link exit0/empty log, factory56/56: tests.json/after-commands.json.
Actual interpreter45/realms30/ownership27/heap31 plus retired-realm PASS,
timeout30s each/exit0/empty stderr: affected-tests.json and individual outputs.
All14 fixed JS fixtures match unchanged checked-in expectations, no reference
regeneration/Uncaught/stderr: fixed-js.json/stdout/stderr.

Full authoritative C/component coverage: full-standards-review.json.
Leading declarations/tabbed arguments/public first sentences, semantic-purpose
paragraphs, immediate fallible checks before publication/teardown, separate
failure/success returns, classifier guards, split conditional/call layout,
actual type/root/exception/lifetime behavior and component boundaries reviewed.
clang-format19.1.7 plus mandatory layout and original constant restoration:
format.json; actual source/object-text hashes: object-code.json. Text changes
are expected for the fixes and regressions.

Supporting checker exit1 solely83 forward shared-cleanup jumps,43 production/
40 fixture, allowed by full C§14; every function has one forward cleanup:
manual-goto-review.json/style-detail.txt/style-summary.txt. No other findings.
Intermediate pointer/fixture compiler rejections and flattened-constant audit
failure are preserved in initial/intermediate logs and intermediate-table-audit.json.
All were resolved before final checks; no rejected source was committed.

No fault injection or whole final ASan/API/independent-client/target/boot campaign
claimed. Those original gates and remaining57 reviews still apply. User-approved
finite q5963h remains active. Main only/all WIP/normal origin/browser3 push;
Issue/Project publication deferred.
