# Browser3 / q590-i01 checkpoint20

Complete full C/component review of dom/removal.c523 and dom/control.c557
original lines. [Inventory](review-inventory.json):113/209 reviewed;
[remaining96 C/header files](remaining.json), zero other files. q590 and whole
p172 remain in-progress; downstream gates stay closed.

Removal review follows separately allocated weak subscriber tokens/FIFO,
Document/token reference counts, finalization in either order, pure non-nesting
notification, stable pre-removal/final insertion/data/split links, prepared
adoption migration, counter overflow and token-only cleanup. Every source
Document keeps its ownership while migrated tokens release their references;
new registry busy/overflow guards cannot strand unpublished allocation because
fresh calloc state has notifying0/references1. Registry matching now uses two
explicit refusal guards with original short-circuit access preserved.

Control review follows clean attributes/text versus owned dirty value,
checked initialization, exact dirty input clone identity, option selection,
normalized ASCII whitespace, caret bounds, native content type mapping and
finalizer release. Public comments and semantic paragraphs comply with full
rules. Fallible text extraction/appends immediately release temporary units
on failure, preserving original partial output and failure codes. The legacy
control_set_value ordering and transactional input_set_value ordering are each
preserved; no unrelated state-policy redesign is introduced.

## Finite verification

- Own GCC14.2.0 plain host rebuild: exit0/warnings0. [Build log](plain-build.log).
- Final native traversal9/9, Range mutation36/36, option state20/20, form owner56/56
  and node cloning26/26:147 checks pass, all exit0/empty stderr.
  Clone checks include actual callee-rooted GC and actual ENOMEM output preservation.
  [Commands/results](tests.json).
- Original C string literals and all control_types initializer entries are
  equal; [table comparison](control-table.json). Both actual object texts differ
  after guarded control-flow changes, including removal assertions' changed
  source line metadata: [hashes](object-code.json). No executable equality claimed.
- Scoped clang-format19.1.7 then mandated definition tabs/compound layout:
  [receipt](format.json). Full manual review accompanies supporting
  [style checker](style-summary.txt),0, and successful `git diff --check`.

No new target/guest, ASan or downstream campaign ran. Remaining96 full reviews
and final whole conformance remain required. GitHub publication is deferred;
all owned build/test processes completed.
