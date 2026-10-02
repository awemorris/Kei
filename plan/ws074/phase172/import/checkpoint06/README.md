# A1 / q584-i01 checkpoint06

Four complete implementation files were reviewed against the current full C
standard and browser component standard: `bind/insertion.c`,
`bind/frame-resource.c`, `bind/style-sheet.c`, and `page/resource.c`.
[Inventory](review-inventory.json) fixes their final hashes. The eight prior
reviewed files have [unchanged hashes](prior-review.json), so their checkpoint04
full reviews remain applicable. Total reviewed: 12 of 209; remaining:
137 C/header and 60 runner/config/data entries. p172 remains in-progress;
whole-Phase clearance and downstream execution are still unavailable.

The review checked file order and one-line static declarations; public/static
purpose comments; ANSI C declaration placement and object-by-object initialization;
call, allocation, check and return conventions; short-circuit order; loop controls,
brace symmetry and paragraph boundaries; protocol counters and state flags;
licenses, production behavior and component ownership. These files contain no
weak collaborators, named sections, goto, locks, function macros or test-only
production switches. The local immutable stylesheet descriptor retains its
process-lifetime callbacks. The XML/CSS model and Document owners remain traced
through the same temporary roots; failed frame initialization still unwinds only
registered roots, and successful publication still precedes old-owner retirement.
No public ABI, Wayland dependency, callback lifetime or resource ownership changed.

Repairs add distinct traversal/decision/ownership paragraphs and split nonstandard
pointer traversal controls. Two local error-code variables now have the name
`error`. [Format evidence](format.json) records clang-format19.1.7 with repository
configuration and restoration of required definition argument tabs and the
readable immutable descriptor row. Style checker reports zero candidates;
`git diff --check` passes. These checks supplement the full review above.

[Object evidence](object-code.json): host GCC14.2.0, production host-builder
flags with `-g0`, `-Wall -Wextra -Werror`; all four compile without diagnostics.
Emitted `.text` equals base0e68854ac in each case. Implementation tokens are
identical after normalizing the two local error-code names. This supports
preserving production behavior for that compiler; it is not target compilation.
The [serialized final plain host build](plain-build.log) passes with no warnings.
No runtime was executed concurrently with building or relinking.

Checkpoint02/03/04 runtime and main target/boot/p014 evidence remains retained for
the unchanged implementation operations. No new target/toolchain build, runtime
suite, install, host modification or push was performed for this checkpoint.
A final target/build/boot decision remains with main during whole-Phase
reconciliation; no repeated native observation is claimed here.
