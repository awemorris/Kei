# p172 checkpoint28 / q590-i01

Complete full C/component review of css/media.c913 original lines.
[Inventory](review-inventory.json):125/209 reviewed;
[remaining84 C/header files](remaining.json), zero other files.

Review covers arena-owned list/query/test allocation, parent-list conjunction,
comma/parenthesis splitting, empty queries, not/only/media type selection,
feature/range/discrete registries, single/double comparisons and reverse
operators, CSS pixel/initial-em/ratio/density conversion, numeric color depths
and actual viewport evaluation. Types now precede both constant tables;
allocation checks precede publication, fallible condition results are checked
before metadata assignment, and multiline evaluation loops have braces.

## Reproduced value validation corrections

[Internal amendment04](../../browser3/q590-amendment-04.md) records trailing-token
acceptance for dimensions/zero/ratio/orientation/discrete values and arbitrary
resolution units treated as dppx. Ten real native cases produce six failures
against actual fdfbe58af engine objects:33/39, exit1.
[Patch](regression-before.patch), [commands](regression-before.json),
[raw failures](media-before.stderr). Complete value consumption and explicit
dppx/x/dpi/dpcm unit recognition now produce39/39; valid controls remain passing.
Primary [MQ4 syntax](https://www.w3.org/TR/mediaqueries-4/#mq-syntax) and
[CSS Values4 resolution units](https://www.w3.org/TR/css-values-4/#resolution)
were checked2026-10-02. This is a narrow correction, not full web conformance.

The already-reviewed host-color-media fixture was fully revalidated, with its
previous checkpoint10 receipt preserved and updated hash; it adds no new review
count. Tokenizer failure releases converted source immediately on failure and
after the checked result on success. Original corpus assertions stay intact.

## Finite verification

- Own GCC14.2.0 plain host rebuild: exit0/warnings0. [Log](plain-build.log).
- Actual native39/39, existing text-transform/media page38 PASS/zero FAIL,
  exit0/empty stderr. [Commands/results](tests.json).
- Actual object text differs after value validation. Original production C
  strings retained; only two density-unit literals, dppx/x, added intentionally.
  [Hashes/delta](object-code.json).
- Scoped clang-format19.1.7 plus required definition tabs/compound layout
  [receipt](format.json); [style checker](style-summary.txt) zero findings;
  full manual review and git diff --check passed.

Existing eight-test cap, legacy unitless density handling and restricted grammar
remain; no new feature or whole Media Queries acceptance is claimed. All209
current hashes reconcile; q590/p172 in-progress, WS incomplete. Remaining84
reviews/final conformance required. No target/guest/ASan or downstream campaign.
GitHub publication deferred; all owned checks ended.
