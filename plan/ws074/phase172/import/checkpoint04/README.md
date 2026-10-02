# P10 generation2 / q579-i01 checkpoint04

Full-file manual review completed for `dom/adopt.c`, `dom/attribute.c`,
`dom/form.c`, `dom/xml.c`, `dom/radio.c`, `dom/select.c`, `xml/dom.h`, and
`xml/xml.h` under the complete C standard and browser component standard.
The exact final hashes and remaining import scope are in
[review-inventory.json](review-inventory.json). This is a bounded reviewed
subset; p172 remains in progress and downstream browser work stays blocked.

The six implementations now explain distinct native decisions, traversal
preparation, actual owner transfers, and the state flags and generation
updates seen by native accessors and Document observers. Pointer traversals
use separate control expressions. XML factories report allocation/validation
failure before their explicit successful return. Native option selectedness
normalization uses an explicit decision while preserving zero and mapping
every nonzero input to one. Callback tables were not reordered in this change.

Manual review checked file organization and declarations; public/static
function comments and order; local declarations and call/decision shape;
allocations and immediate checks; complete initialization; short-circuit order;
paragraph intent and spacing; protocol flags/counters; success/absence/refusal
returns; licenses and default production behavior. These six files have no
weak optional collaborators, named sections, locks, goto, function macros,
or test-only production switches. Attribute namespace tables are immutable
for the process lifetime. XML Document/optional target roots survive factory
collection and are removed individually on failures. Adoption preserves node
identity and migrates prepared weak subscriptions before owner publication.
Form/radio/select membership uses current ordinary trees and exact namespace
identity; template-content graphs remain independently owned. The reviewed
XML headers retain immutable model-owned records and borrowed-unit lifetimes.
No public browser ABI, Wayland boundary, resource ownership or lifetime changed.

[Formatter comparison](format.json): clang-format19.1.7 with checked-in
configuration, followed by restoration of required definition argument tabs,
decision clause lines and readable immutable initializer rows.
[Style checker](style-summary.txt): zero candidates. These are supplemental
checks; the review above supplies the semantic assessment for this subset.

[Object-code comparison](object-code.json): GCC14.2.0 with host builder flags,
`-g0`, `-Wall -Wextra -Werror`; six files compile with zero warnings and their
emitted `.text` is identical to base25729c88a. This supports preservation of
behavior, including the two return/Boolean rewrites, for this compiler only.
It is not a target compiler proof. The production changes are equivalent
control-flow spelling, comments and whitespace; no feature implementation is
added. Main's checkpoint03 target/boot/p014 observations remain applicable to
these equivalent production operations, with no claim of a repeated native run.

The serialized [plain host build](plain-build.log) passes with zero warnings.
[Affected tests](tests.json): 11 existing groups all pass (native namespaces,
forms, cloning, radio, select/option state and collections, native XML nodes,
projection and bindings). Build and tests did not relink concurrently. The final
select comment/clause-layout adjustment has identical executable tokens to its
built form; the object-code check compiled the final sources. Existing broader
plain/ASan/component/golden/Acid2 evidence is retained at checkpoints02/03.
No optional broad test repetition, target/toolchain build, host install or push.

Remaining work is explicit in the inventory. Unreviewed C files are not accepted
because they passed the checker. Known concrete remaining gaps include
`bind/svg-length.c` and other callback table files: declarations required by
constant initializers may precede their tables under main's user-approved
8367481d9 rule, while non-initializer helpers must move after the variables.
`bind/insertion.c` still has unsplit pointer traversal controls; `page/resource.c`
still groups some distinct decisions and traversals without their own purpose
paragraphs. `bind/frame-resource.c` needs complete root/publication/fallible-call
paragraph review. New tests and the larger modified bind/CSS/HTML/JS/page/VM
implementations also need full final review. No WS107 relocation exception applies.
