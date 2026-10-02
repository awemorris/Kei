# A1 / q584-i01 checkpoint11

Complete full C/component review of host-data-url.c, host-image-zero.c,
host-namespace.c and host-radio-state.c covers their entire native fixture code.
[Inventory](review-inventory.json): reviewed88/209; remaining121 C/header.
p172/q584 stay in-progress and downstream browser work remains blocked.

Review covers section/declaration/prototype order, genuine DOM/VM/native inputs,
explicit roots and conservative construction protection, arena/owned response
cleanup, independent namespace/clone/dirty-state storage, counters, allocations,
call/decision/return shape, purpose paragraphs, immutable corpus lifetime,
licenses and default production behavior. Geometry fixtures provide controlled
stack-owned native box inputs over genuine DOM nodes; they exercise production
sizing/rectangle APIs without claiming a full renderer test or replacing code.

Repairs clarify distinct native observations and ownership/report stages, use
error-code names and explicit Boolean decisions, split multiline calls one
argument per line, and add equivalent final void returns. [Equivalence]
(equivalence.json) records executable token equality for data/radio after exact
spelling normalizations; every script literal and short-circuit operand/order is
preserved. Radio's and namespace's GCC object text equals the base; data's
explicit Boolean/refusal spelling produces different compiler instructions, so
no identical-object claim is made for that file.

## Limited fixture failure handling

Main clarified during q584 execution that necessary in-scope standards repairs
include ordinary fixture failure handling. This is the existing approved Queue's
technical scope, with no production-engine OOM change or new browser feature.
Two observed gaps were fixed under that limited instruction: namespace failure
reports save fprintf's result and immediately return if negative, retaining the
already-added failure count; zero-image fixtures allocate width/height value
atoms separately and reject NULL with existing construction failure2 before the
setter. These explicit atom refusal outcomes alter low-memory fixture behavior;
they are disclosed instead of being called style-only or borrowing old runtime
proof. Production API, ownership and behavior remain unchanged. The new NULL and
negative-report branches were reviewed but not fault-injected.

[Formatter](format.json): clang-format19.1.7, with required definition argument
tabs, immutable corpus rows and split-call arguments restored. [Style checker]
(style-summary.txt):zero; git diff-check passes. [Final own serialized plain
build](plain-build.log) and four final independent compiles pass warning0.
[Four actual native fixtures](tests.json) pass102 observations with empty stderr.
[Object comparison](object-code.json) reports exact results without weakening
whole-Phase acceptance. Existing ASan/target/boot evidence remains retained, but
is not claimed as an executed test of the two newly spelled failure branches.

The [initial failed build](plain-build-initial-failed.log) is retained: a layout
edit incorrectly matched a delimiter inside fixture JavaScript strings. The call
layout was corrected using quote-aware scanning, script tokens rechecked against
the base, then the complete build and all four fixtures rerun successfully.
No toolchain build, install, push, aggregate make-check or serial-log judgment.
