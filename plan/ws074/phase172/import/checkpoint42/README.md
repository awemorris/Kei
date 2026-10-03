# p172 checkpoint42 / q596-i01

Full C/component review of host-realms879 original lines and
host-context-ownership779 original lines. Review154/209, remaining55 C/header;
all209 source hashes reconcile against checkpoint41. Whole q596/p172 item remains
in-progress, WS incomplete, p100 unselected.

Actual operations now check their result before successful realm/heap teardown
or UTF-16 source release; error paths still unwind acquired resources immediately.
Ownership reporting retains failed fprintf observations. Successful saved function
and suspended generator results are branded before exposing their native cells.
Semantic paragraphs separate global publication, async jobs, construction target
fallback, exception transport, actual source metadata and errno state restoration.
Dedicated test refusal natives still return literal ENOMEM/ENOTSUP; result comments
identify the successful fixture purpose and explicit refusal contract.

Full original realm/ownership behavior inspected: bidirectional own-realm closure,
class/derived/new.target/default intrinsic identity, native reentry, generators,
async jobs, source throws and direct C APIs/stack exhaustion; managed saved function/
global/prototype/Document/generator edges and exact eventual host release, observer
cycles, detached tasks, borrowed timer removes last connection during actual GC,
bounded construction failure/unsupported async host and whole-heap finalization.
Original C strings, assertions, scripts and construction/conservative root exclusion
remain; no production edits or extra retaining roots during tested GC.

Scoped GCC14.2 -Wall/-Wextra/-Werror compile/link exit0/empty log against actual
current own engine objects: before/after-commands.json, compile-link.log,
build-result.json/versions.txt. Actual realm30/30 and ownership27/27 each
bounded timeout30s/exit0/empty stderr: tests.json/individual outputs.
clang-format19.1.7/restored tabbed definitions/condition/call layout: format.json.
Supporting checker exit0/findings0: style-summary.txt/style-detail.txt;
full actual source/authoritative standards coverage: full-standards-review.json.
Old/final source and actual object-text hashes and equal C literals: object-code.json.
Object text differs from the checked guard/error/teardown changes; all209 hashes match.

Whole final target/boot/API/independent-client/ASan and55 remaining full reviews
are not claimed. No new fault campaign. q596 finite3h/main only/all WIP/normal
origin/browser3 push; Issues/Project deferred. Previous build cleanup complete.
